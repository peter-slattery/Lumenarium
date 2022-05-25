File_Async_Job_System
os_file_jobs_init()
{
  open_files_init();
  File_Async_Job_System result = file_async_jobs_init(os_file_async_work_on_job);
  return result;
}

File_Handle 
os_file_open(String path, File_Access_Flags flags_access,  File_Create_Flags flags_create)
{
  File_Handle result = {};

  s32 flags = 0;
  if (has_flag_exact(flags_access, (FileAccess_Read | FileAccess_Write)))
  {
    add_flag(flags, O_RDWR);
  }
  else
  {
    if (has_flag(flags_access, FileAccess_Read))  
    {
      add_flag(flags, O_RDONLY);
    }
    else if (has_flag(flags_access, FileAccess_Write)) 
    {
      add_flag(flags, O_WRONLY); 
    }
    else
    {
      return result;
    } 
  }

  switch (flags_create)
  {
    case FileCreate_New:          { add_flag(flags, O_CREAT | O_EXCL ); } break;
    case FileCreate_CreateAlways: { add_flag(flags, O_CREAT); } break;
    case FileCreate_OpenExisting: { /* add_flag(flags, O_); */ } break;
    case FileCreate_OpenAlways:   { /* add_flag(flags, O_); */ } break;
    invalid_default_case;
  }
  
  s32 file_handle = open((char*)path.str, flags);
  if (file_handle >= 0)
  {
    result = open_files_put_handle(file_handle, path);
  }
  else
  {
    s32 errsv = errno;
    printf("Error: os_file_open - %d\n", errsv);
    printf("\tAttempting to open: %.*s\n", str_varg(path));
    printf("\tFlags: %u %u\n", flags_access, flags_create);
  }
  return result;
}

void        
os_file_close(File_Handle file_handle)
{
  s32 os_handle = open_files_get_handle(file_handle);
  if (close(os_handle) != -1)
  {
    open_files_rem_file(file_handle);
  }
  else
  {
    s32 errsv = errno;
    printf("Error: os_file_close - %d\n", errsv);
  }
}

File_Info   
os_file_get_info(File_Handle file_handle, Allocator* allocator)
{
  File_Info info = {};
  s32 os_handle = open_files_get_handle(file_handle);
  if (os_handle != -1)
  {
    String path = open_files_get_path(file_handle);

    struct stat os_info = {};
    if (fstat(os_handle, &os_info) != -1)
    {
      info.path = string_copy(path, allocator);
      info.path_abs = allocator_alloc_string(allocator, PATH_MAX);
      if (realpath((char*)path.str, (char*)info.path_abs.str) != 0)
      {
        info.path_abs.len = c_str_len((char*)info.path_abs.str);
      }
      else
      {
        s32 errsv = errno;
        printf("Error - os_file_get_info - %d - realpath\n", errsv);
      }
      info.size = (u64)os_info.st_size;
      if (S_ISDIR(os_info.st_mode))
      {
        add_flag(info.flags, FileFlag_IsDir);
      }
      else if (!S_ISREG(os_info.st_mode))
      {
        printf("Error - os_file_get_info - stat-ing a handle that is not a directory or a file\n");
      }
    }
    else
    {
      s32 errsv = errno;
      printf("Error: os_file_get_info - %d\n", errsv);
    }
  }
  return info;
}

Data        
os_file_read_all(File_Handle file_handle, Allocator* allocator)
{
  Data result = {};
  s32 os_handle = open_files_get_handle(file_handle);
  if (os_handle == -1) return result;

  // get file size
  s32 offset = lseek(os_handle, 0, SEEK_END);
  if (offset == -1) 
  {
    s32 errsv = errno;
    printf("Error: os_file_read_all:lseek - %d\n", errsv);
    return result;
  }
  lseek(os_handle, 0, SEEK_SET);

  result.base = allocator_alloc(allocator, offset + 1);
  result.size = offset + 1;

  s32 bytes_read = read(os_handle, result.base, result.size);
  if (bytes_read == (result.size - 1))
  {
    result.base[bytes_read] = 0; // null term
  }
  else if (bytes_read >= 0)
  {
    printf("Error: os_file_read:read - whole file not read.\n");
  }
  else if (bytes_read == -1)
  {
    s32 errsv = errno;
    printf("Error: os_file_read_all:read - %d\n", errsv);
  }
  
  return result;
}

bool
os_file_write_(s32 os_handle, Data file_data)
{
  s32 size_written = write(os_handle, file_data.base, file_data.size);
  if (size_written > 0 && size_written != file_data.size)
  {
    printf("Error: os_file_write_all:write - whole file not written\n");
    return true;
  }
  else if (size_written < 0)
  {
    osx_err_print("write");
    return false;
  }

  return true;
}

bool        
os_file_write_all(File_Handle file_handle, Data file_data)
{
  s32 os_handle = open_files_get_handle(file_handle);
  if (os_handle == -1) return false;

  lseek(os_handle, 0, SEEK_SET);
  return os_file_write_(os_handle, file_data);
}

bool
os_file_write(File_Handle file_handle, Data file_data)
{
  s32 os_handle = open_files_get_handle(file_handle);
  if (os_handle == -1) return false;

  return os_file_write_(os_handle, file_data);
}

String      
os_get_exe_path(Allocator* allocator)
{
  u32 needed = 0;
  _NSGetExecutablePath(0, &needed);

  String result = allocator_alloc_string(allocator, needed + 1);
  
  u32 cap = (u64)result.cap;
  s32 r = _NSGetExecutablePath((char*)result.str, &cap);
  if (r == 0)
  {
    result.len = cap;
    result.str[result.len] = 0;
  } 

  return result;
}

bool        
os_pwd_set(String path)
{
  s32 result = chdir((char*)path.str);
  if (result == -1) 
  {
    osx_err_print("chdir");
    return false;
  }
  return true;
}

#if 0
File_Info_List 
os_dir_enum(String path, Platform_Enum_Dir_Flags flags, Allocator* allocator)
{

}
#endif
