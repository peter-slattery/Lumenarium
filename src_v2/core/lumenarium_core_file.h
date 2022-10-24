#if !defined(LUMENARIUM_CORE_FILE_H)
#define LUMENARIUM_CORE_FILE_H

typedef struct File_Handle File_Handle;
struct File_Handle
{
  u64 value;
};

typedef u32 File_Access_Flags;
enum
{
  FileAccess_None  = 0,
  FileAccess_Read  = 1,
  FileAccess_Write = 2,
};

typedef u32 File_Create_Flags;
enum
{
  // these match https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
  FileCreate_None = 0,

  // Creates a new file, only if it does not already exist.
  // If the file exists, an error is returned
  FileCreate_New = 1,

  // Creates a new file, always.
  // If the specified file exists and is writable, the file is overwritten
  FileCreate_CreateAlways = 2,

  // Opens a file or device, only if it exists.
  FileCreate_OpenExisting = 3,

  // Opens a file, always.
  FileCreate_OpenAlways = 4,
};

typedef u32 File_Flags;
enum
{
  FileFlag_IsFile = 0,
  FileFlag_IsDir = 1,
};

typedef struct File_Info File_Info;
struct File_Info
{
  String path;
  String path_abs;
  u64 size;
  u64 time_created;
  u64 time_last_write;
  File_Flags flags;
};

typedef struct File_Info_List_Ele File_Info_List_Ele;
struct File_Info_List_Ele
{
  File_Info info;
  File_Info_List_Ele* next;
};

typedef struct File_Info_List File_Info_List;
struct File_Info_List
{
  File_Info_List_Ele* first;
  File_Info_List_Ele* last;
};

// For Cross Platform File Operations use these:

typedef u32 File_Async_Job_Flags;
enum 
{
  FileAsyncJob_Invalid  = 0,
  FileAsyncJob_Read     = 1 << 0,
  FileAsyncJob_Write    = 1 << 1,
  FileAsyncJob_InFlight = 1 << 2,
  FileAsyncJob_Success  = 1 << 3,
  FileAsyncJob_Failed   = 1 << 4,
};

typedef struct File_Async_Job_Args File_Async_Job_Args;
struct File_Async_Job_Args
{
  String path;
  Data data;
  File_Async_Job_Flags flags;
  u32 error;
};

typedef void File_Async_Cb(File_Async_Job_Args args, u8* user_data);

typedef struct File_Async_Job File_Async_Job;
struct File_Async_Job
{
  Data job_memory;
  File_Async_Job_Args args;
  File_Async_Cb* cb;
};

typedef void File_Async_Job_System_Do_Job(File_Async_Job* job);

typedef struct File_Async_Job_System File_Async_Job_System;
struct File_Async_Job_System
{
  File_Async_Job_System_Do_Job* do_job;
};

global Allocator* file_jobs_arena = 0;
#define FILE_ASYNC_MAX_JOBS 32
global File_Async_Job file_async_jobs[FILE_ASYNC_MAX_JOBS];
global u32 file_async_jobs_len = 0;

internal File_Async_Job_System file_async_jobs_init(File_Async_Job_System_Do_Job* do_job);
internal bool file_async_job_add(File_Async_Job job);
internal File_Async_Job file_async_job_rem(u64 index);
internal bool file_async_read(String path, File_Async_Cb* cb);
internal bool file_async_write(String path, Data data, File_Async_Cb* cb);
internal void file_async_job_complete(File_Async_Job* job, u8* user_data);
internal void file_async_jobs_do_work(File_Async_Job_System* system, u64 max_jobs, u8* user_data);
internal void file_async_jobs_do_work(File_Async_Job_System* system, u64 max_jobs, u8* user_data);

typedef u32 Platform_Enum_Dir_Flags;
enum
{
  EnumDir_Recursive = 1,
  EnumDir_IncludeDirectories = 2,
};

///////////////////////////////////////
///////////////////////////////////////
//      IMPLEMENTATION
///////////////////////////////////////
///////////////////////////////////////

internal File_Async_Job_System
file_async_jobs_init(File_Async_Job_System_Do_Job* do_job)
{
  file_jobs_arena = paged_allocator_create_reserve(MB(4), 256);
  File_Async_Job_System result = {};
  result.do_job = do_job;
  return result;
}

internal bool
file_async_job_add(File_Async_Job job)
{
  if (file_async_jobs_len >= FILE_ASYNC_MAX_JOBS) return false;
  
  // Copy data to job local memory
  u64 size_needed = job.args.path.len + job.args.data.size + 1;
  u8* job_mem = allocator_alloc(file_jobs_arena, size_needed);
  String job_path = string_create(job_mem, 0, job.args.path.len + 1);
  u64 copied = string_copy_to(&job_path, job.args.path);
  Data job_data = data_create(job_mem + job_path.cap + 1, size_needed - (job_path.cap + 1));
  memory_copy(job.args.data.base, job_data.base, job.args.data.size);
  job.args.path = job_path;
  job.args.data = job_data;
  job.job_memory = data_create(job_mem, size_needed);
  
  file_async_jobs[file_async_jobs_len++] = job;
  return true;
}

internal File_Async_Job
file_async_job_rem(u64 index)
{
  assert(index < file_async_jobs_len);
  File_Async_Job result = file_async_jobs[index];
  
  file_async_jobs_len -= 1;
  if (file_async_jobs_len > 0)
  {
    u32 last_job = file_async_jobs_len;
    file_async_jobs[index] = file_async_jobs[last_job];
  }
  
  return result;
}

internal bool
file_async_read(String path, File_Async_Cb* cb)
{
  File_Async_Job job = {};
  job.args.path = path;
  job.args.flags = (
                    FileAsyncJob_Read |
                    FileAsyncJob_InFlight
                    );
  job.cb = cb;
  bool result = file_async_job_add(job);
  return result;
}

internal bool
file_async_write(String path, Data data, File_Async_Cb* cb)
{
  File_Async_Job job = {};
  job.args.path = path;
  job.args.data = data;
  job.args.flags = (
                    FileAsyncJob_Write |
                    FileAsyncJob_InFlight
                    );
  job.cb = cb;
  bool result = file_async_job_add(job);
  return result;
}

internal void
file_async_job_complete(File_Async_Job* job, u8* user_data)
{
  job->cb(job->args, user_data);
  allocator_free(file_jobs_arena, job->job_memory.base, job->job_memory.size); 
  if (has_flag(job->args.flags, FileAsyncJob_Write))
  {
    allocator_free(file_jobs_arena, job->args.data.base, job->args.data.size);
  }
}

internal void
file_async_jobs_do_work(File_Async_Job_System* system, u64 max_jobs, u8* user_data)
{
  assert(system->do_job);

  u64 to_do = max_jobs;
  if (max_jobs > file_async_jobs_len) to_do = file_async_jobs_len;
  
  File_Async_Job_Flags completed = (
                                   FileAsyncJob_Success | 
                                   FileAsyncJob_Failed
                                   );
  
  for (u64 i = to_do - 1; i < to_do; i--)
  {
    File_Async_Job* job = file_async_jobs + i;
    system->do_job(job);
    if (has_flag(job->args.flags, completed))
    {
      if (has_flag_exact(job->args.flags, FileAsyncJob_Success))
      {
        file_async_job_complete(job, user_data);
      }
      file_async_job_rem(i);
    }
  }
}


#endif // LUMENARIUM_CORE_FILE_H