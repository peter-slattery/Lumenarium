#define win32_open_files_cap   32
char    win32_open_file_paths[win32_open_files_cap][MAX_PATH];
u64     win32_open_files_len = 1; // zero is invalid
HANDLE  win32_open_files[win32_open_files_cap];

void
win32_files_init()
{
  for (u32 i = 0; i < win32_open_files_cap; i++)
  {
    win32_open_files[i] = INVALID_HANDLE_VALUE;
  }
}

HANDLE
win32_get_open_file_handle(Platform_File_Handle file)
{
  return win32_open_files[file.value];
}

internal u64
win32_high_low_to_u64(u32 low_part, u32 high_part)
{
  ULARGE_INTEGER Time = {};
  Time.LowPart = low_part;
  Time.HighPart = high_part;
  u64 result = Time.QuadPart;
  return result;
}

internal u64
win32_file_time_to_u64(FILETIME file_time)
{
  return win32_high_low_to_u64(file_time.dwLowDateTime, file_time.dwHighDateTime);
}

Platform_File_Handle
platform_file_open(String path, Platform_File_Access_Flags flags_access,  Platform_File_Create_Flags flags_create)
{
  Platform_File_Handle result = {};
  
  DWORD flags_create_ = OPEN_EXISTING;
  
  HANDLE file_handle = CreateFileA(
                                   (char*)path.str, 
                                   (DWORD)flags_access,
                                   0, // share mode
                                   NULL, // security attributes
                                   (DWORD)flags_create,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL
                                   );
  
  if (file_handle != INVALID_HANDLE_VALUE)
  {
    if (win32_open_files_len < win32_open_files_cap)
    {
      result.value = win32_open_files_len++;
    }
    else
    {
      // search for emtpy index
      for (u32 i = 1; i < win32_open_files_cap; i++)
      {
        if (win32_open_files[i] == INVALID_HANDLE_VALUE)
        {
          result.value = i;
        }
      }
    }
    
    assert(result.value != 0);
    win32_open_files[result.value] = file_handle;
    
    memory_copy(path.str, (u8*)win32_open_file_paths[result.value], path.len);
    win32_open_file_paths[result.value][path.len] = 0; // null term
  }
  else
  {
    win32_get_last_error();
  }
  
  return result;
}

void
platform_file_close(Platform_File_Handle file_handle)
{
  assert(file_handle.value < win32_open_files_len);
  CloseHandle(win32_get_open_file_handle(file_handle));
  win32_open_files[file_handle.value] = INVALID_HANDLE_VALUE;
}

u64
win32_get_file_size(HANDLE h)
{
  DWORD size_low, size_high;
  size_low = GetFileSize(h, &size_high);
  if (size_low == INVALID_FILE_SIZE)
  {
    win32_get_last_error();
    return 0;
  }
  LARGE_INTEGER win32_size;
  win32_size.LowPart = size_low;
  win32_size.HighPart = size_high;
  return (u64)win32_size.QuadPart;
}

Platform_File_Info
platform_file_get_info(Platform_File_Handle file_handle, Allocator* allocator)
{
  Platform_File_Info result = {};
  HANDLE h = win32_get_open_file_handle(file_handle);
  if (h == INVALID_HANDLE_VALUE) return result;
  
  // File Size
  u64 win32_size = win32_get_file_size(h);
  if (win32_size == 0 && win32_last_error != 0) return result;
  
  // File Times
  FILETIME time_created, time_last_write;
  if (!GetFileTime(h, &time_created, (LPFILETIME)0, &time_last_write))
  {
    win32_get_last_error();
    return result;
  }
  
  // File Path
  // call GetFullPathName with empty dest just to get the length needed
  DWORD file_name_len_needed = GetFullPathName(
                                               win32_open_file_paths[file_handle.value], 0, 0, 0
                                               );
  if (!file_name_len_needed)
  {
    win32_get_last_error();
    return result;
  }
  
  result.path = allocator_alloc_string(allocator, (u64)file_name_len_needed);
  result.path.len = (u64)GetFullPathName(
                                         win32_open_file_paths[file_handle.value],
                                         (DWORD)result.path.cap,
                                         (char*)result.path.str,
                                         0
                                         );
  result.path_abs = result.path;
  
  // File Attributes
  DWORD file_attrs = GetFileAttributesA((char*)result.path.str);
  if (!file_attrs)
  {
    win32_get_last_error();
    return result;
  }
  
  result.size = win32_size;
  result.time_created = win32_file_time_to_u64(time_created);
  result.time_last_write = win32_file_time_to_u64(time_last_write);
  
  if (has_flag(file_attrs, FILE_ATTRIBUTE_DIRECTORY)) 
  {
    add_flag(result.flags, FileFlag_IsDir);
  }
  
  return result;
}

Data
platform_file_read_all(Platform_File_Handle file_handle, Allocator* allocator)
{
  Data result = {};
  
  HANDLE h = win32_get_open_file_handle(file_handle);
  if (h == INVALID_HANDLE_VALUE) return result;
  
  u64 file_size = win32_get_file_size(h);
  if (file_size == 0 && win32_last_error != 0) return result;
  
  result.base = allocator_alloc(allocator, file_size + 1);
  result.size = file_size + 1;
  
  DWORD bytes_read = 0;
  if (ReadFile(h, (void*)result.base, (DWORD)result.size, &bytes_read, NULL))
  {
    result.base[result.size - 1] = 0;
  }
  else
  {
    win32_get_last_error();
  }
  
  return result;
}

bool
platform_file_write_all(Platform_File_Handle file_handle, Data file_data)
{
  bool result = false;
  
  HANDLE h = win32_get_open_file_handle(file_handle);
  if (h == INVALID_HANDLE_VALUE) return result;
  
  // Set file pointer to beginning
  SetFilePointer(h, 0, 0, FILE_BEGIN);
  
  DWORD bytes_written = 0;
  if (WriteFile(h, file_data.base, (DWORD)file_data.size, &bytes_written, NULL))
  {
    result = (bytes_written == file_data.size);
  }
  
  return result;
}

void
platform_dir_enum_(String path, Platform_Enum_Dir_Flags flags, Allocator* allocator, Platform_File_Info_List* list)
{
  WIN32_FIND_DATA ffd;
  HANDLE search_handle = FindFirstFile((char*)path.str, &ffd);
  if (search_handle != INVALID_HANDLE_VALUE)
  {
    do
    {
      Platform_File_Info_List_Ele* ele = allocator_alloc_struct(
                                                                allocator, Platform_File_Info_List_Ele
                                                                );
      
      sll_push(list->first, list->last, ele);
    } while (FindNextFile(search_handle, &ffd));
  }
}

Platform_File_Info_List
platform_dir_enum(String path, Platform_Enum_Dir_Flags flags, Allocator* allocator)
{
  Platform_File_Info_List result = {};
  platform_dir_enum_(path, flags, allocator, &result);
  return result;
}

String
platform_get_exe_path(Allocator* allocator)
{
  String result = allocator_alloc_string(allocator, (u64)MAX_PATH);
  result.len = (u64)GetModuleFileName(NULL, (char*)result.str, (DWORD)result.cap);
  if (!result.len) return result;
  
  return result;
}

bool
platform_pwd_set(String path)
{
  bool result = SetCurrentDirectory((char*)path.str);
  if (!result) win32_get_last_error();
  return result;
}

void
platform_file_async_work_on_job(Platform_File_Async_Job* job)
{
  Platform_File_Handle file = {};
  if (has_flag(job->args.flags, PlatformFileAsyncJob_Read))
  {
    file = platform_file_open(job->args.path, FileAccess_Read, FileCreate_OpenExisting);
    Data result = platform_file_read_all(file, platform_file_jobs_arena);
    if (result.base != 0) 
    {
      job->args.data = result;
      add_flag(job->args.flags, PlatformFileAsyncJob_Success);
    }
    else
    {
      add_flag(job->args.flags, PlatformFileAsyncJob_Failed);
    }
  }
  else if (has_flag(job->args.flags, PlatformFileAsyncJob_Write))
  {
    file = platform_file_open(job->args.path, FileAccess_Write, FileCreate_OpenAlways);
    if (platform_file_write_all(file, job->args.data))
    {
      add_flag(job->args.flags, PlatformFileAsyncJob_Success);
    }
    else
    {
      add_flag(job->args.flags, PlatformFileAsyncJob_Failed);
    }
  }
  platform_file_close(file);
}