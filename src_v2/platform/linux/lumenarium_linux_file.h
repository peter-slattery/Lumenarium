#ifndef LUMENARIUM_LINUX_FILE_H
#define LUMENARIUM_LINUX_FILE_H 1

File_Async_Job_System 
os_file_jobs_init()
{

}

File_Handle 
os_file_open(String path, File_Access_Flags flags_access,  File_Create_Flags flags_create)
{
  invalid_code_path;
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
}

Data        
os_file_read_all(File_Handle file_handle, Allocator* allocator)
{
  invalid_code_path;
}

bool        
os_file_write_all(File_Handle file_handle, Data file_data)
{
  invalid_code_path;
}


String      
os_get_exe_path(Allocator* allocator)
{
  invalid_code_path;
}

bool        
os_pwd_set(String path)
{
  invalid_code_path;
}


File_Info_List 
os_dir_enum(String path, Platform_Enum_Dir_Flags flags, Allocator* allocator)
{
  invalid_code_path;
}

void        
os_file_async_work_on_job(File_Async_Job* job)
{

}

#endif // LUMENARIUM_LINUX_FILE_H