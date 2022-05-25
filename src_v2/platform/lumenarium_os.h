#if !defined(LUMENARIUM_OS)
#define LUMENARIUM_OS

///////////////////////////////////////
//    Memory

u8*  os_mem_reserve(u64 size);
u8*  os_mem_commit(u8* base, u64 size);
bool os_mem_decommit(u8* base, u64 size);
bool os_mem_release(u8* base, u64 size);

///////////////////////////////////////
//    File I/O

File_Async_Job_System os_file_jobs_init();
File_Handle os_file_open(String path, File_Access_Flags flags_access,  File_Create_Flags flags_create);
void        os_file_close(File_Handle file_handle);
File_Info   os_file_get_info(File_Handle file_handle, Allocator* allocator);
Data        os_file_read_all(File_Handle file_handle, Allocator* allocator);
bool        os_file_write_all(File_Handle file_handle, Data file_data);
bool        os_file_write(File_Handle file_handle, Data file_data);

String      os_get_exe_path(Allocator* allocator);
bool        os_pwd_set(String path);

File_Info_List os_dir_enum(String path, Platform_Enum_Dir_Flags flags, Allocator* allocator);

// Asnyc Jobs
void        os_file_async_work_on_job(File_Async_Job* job);

///////////////////////////////////////
//    Time

Ticks os_get_ticks();
r64   os_get_ticks_per_second();

///////////////////////////////////////
//    Threads

Thread_Handle os_thread_begin(Thread_Proc* proc, u8* user_data);
void          os_thread_end(Thread_Handle thread_handle);

u32 os_interlocked_increment(volatile u32* value);
u32 os_interlocked_cmp_exchg(volatile u32* dest, u32 new_value, u32 old_value);

///////////////////////////////////////
//    Network Access

Socket_Handle os_socket_create(s32 domain, s32 type, s32 protocol);
bool          os_socket_bind();
bool          os_socket_connect();
bool          os_socket_close();
Data          os_socket_recv();
s32           os_socket_set_listening();
s32           os_socket_send();
s32           os_socket_send_to(Socket_Handle handle, u32 addr, u32 port, Data data, s32 flags);
s32           os_socket_set_opt(Socket_Handle handle, int level, int option_name,
u8* option_value, s32 option_len);

void open_sockets_init();

#endif // LUMENARIUM_OS