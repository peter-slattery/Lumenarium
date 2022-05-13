#ifndef LUMENARIUM_RASPI_THREADS_H
#define LUMENARIUM_RASPI_THREADS_H 1

Thread_Handle 
os_thread_begin(Thread_Proc* proc, u8* user_data)
{
  invalid_code_path;
  return Thread_Handle{0};
}

void          
os_thread_end(Thread_Handle thread_handle)
{
  invalid_code_path;
}

u32 
os_interlocked_increment(volatile u32* value)
{
  invalid_code_path;
  return 0;
}

u32 
os_interlocked_cmp_exchg(volatile u32* dest, u32 new_value, u32 old_value)
{
  invalid_code_path;
  return 0;
}


#endif // LUMENARIUM_RASPI_THREADS_H