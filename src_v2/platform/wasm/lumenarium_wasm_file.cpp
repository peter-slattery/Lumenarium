
WASM_EXTERN u32 wasm_fetch(char* file_path, u32 file_path_len, u8* dest, u32 size);

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

