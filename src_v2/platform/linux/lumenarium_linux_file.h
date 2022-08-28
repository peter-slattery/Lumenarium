#ifndef LUMENARIUM_LINUX_FILE_H
#define LUMENARIUM_LINUX_FILE_H 1

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
  
  mode_t mode = 0;
  
  s32 flags = 0;
  if(has_flag_exact(flags_access, (FileAccess_Read | FileAccess_Write)))
  {
    add_flag(flags, O_RDWR);
  }
  else
  {
    if (has_flag(flags_access, FileAccess_Read)) {
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
  
  // This additional argument is required if we're creating a file
  // if we want this process to be able to open it again later.
  mode_t create_flags = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  
  switch (flags_create)
  {
    case FileCreate_New: 
    { 
      add_flag(flags, O_CREAT | O_EXCL ); 
      mode = create_flags;
    } break;
    case FileCreate_OpenExisting: { /* add_flag(flags, O_); */ } break;
    case FileCreate_CreateAlways:
    case FileCreate_OpenAlways:
    { 
      add_flag(flags, O_CREAT); 
      mode = create_flags;
    } break;
    invalid_default_case;
  }
  
  s32 file_handle = open((char*)path.str, flags, mode);
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

bool
os_file_delete(String file_path)
{
  if (remove((char*)file_path.str) != 0) {
    perror("Error deleting file\n");
    return false;
  }
  return true;
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
    linux_err_print("write");
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
  String result = {};
  char buf[PATH_MAX + 1];
  s32 r = readlink("/proc/self/exe", buf, PATH_MAX);
  if (r != -1)
  {
    u64 needed = c_str_len(buf);
    result = allocator_alloc_string(allocator, needed + 1);
    memory_copy((u8*)buf, result.str, needed);
    result.len = needed;
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
    linux_err_print("chdir");
    return false;
  }
  return true;
}


File_Info_List 
os_dir_enum(String path, Platform_Enum_Dir_Flags flags, Allocator* allocator)
{
  invalid_code_path;
  return (File_Info_List){};
}

void        
os_file_async_work_on_job(File_Async_Job* job)
{
  
}

#endif // LUMENARIUM_LINUX_FILE_H