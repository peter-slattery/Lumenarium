
WASM_EXTERN u32 wasm_fetch(char* file_path, u32 file_path_len, u8* dest, u32 size);
WASM_EXTERN void wasm_platform_file_async_work_on_job(char* path, u32 path_len, u8* data, u32 data_len, bool read, bool write);

Platform_File_Handle
platform_file_open(String path, Platform_File_Access_Flags flags_access,  Platform_File_Create_Flags flags_create)
{
  return {};
}

void
platform_file_close(Platform_File_Handle file_handle)
{
  return;
}

Platform_File_Info
platform_file_get_info(Platform_File_Handle file_handle, Allocator* allocator)
{
  return {};
}

Data
platform_file_read_all(Platform_File_Handle file_handle, Allocator* allocator)
{
  return {};
}

bool
platform_file_write_all(Platform_File_Handle file_handle, Data file_data)
{
  return {};
}

String
platform_get_exe_path(Allocator* allocator)
{
  return {};
}

bool
platform_pwd_set(String path)
{
  return false;
}

void
platform_file_async_work_on_job(Platform_File_Async_Job* job)
{
  wasm_platform_file_async_work_on_job(
                                       (char*)job->args.path.str, job->args.path.len, 
                                       job->args.data.base, job->args.data.size,
                                       has_flag(job->args.flags, PlatformFileAsyncJob_Read),
                                       has_flag(job->args.flags, PlatformFileAsyncJob_Write)
                                       );
}