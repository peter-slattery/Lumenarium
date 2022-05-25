#ifndef LUMENARIUM_LINUX_FILE_H
#define LUMENARIUM_LINUX_FILE_H 1

File_Async_Job_System 
os_file_jobs_init()
{
  return (File_Async_Job_System){};
}

File_Handle 
os_file_open(String path, File_Access_Flags flags_access,  File_Create_Flags flags_create)
{
  invalid_code_path;
  return (File_Handle){};
}

void        
os_file_close(File_Handle file_handle)
{
  invalid_code_path;
}

File_Info   
os_file_get_info(File_Handle file_handle, Allocator* allocator)
{
  invalid_code_path;
  return (File_Info){};
}

Data        
os_file_read_all(File_Handle file_handle, Allocator* allocator)
{
  invalid_code_path;
  return (Data){};
}

bool        
os_file_write_all(File_Handle file_handle, Data file_data)
{
  invalid_code_path;
  return false;
}

bool
os_file_write(File_Handle file_handle, Data file_data)
{
  invalid_code_path;
  return false;
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