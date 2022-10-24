
Platform_Thread_Handle
platform_thread_begin(Platform_Thread_Proc* proc, u8* user_data)
{
  return {};
}

void
platform_thread_end(Platform_Thread_Handle thread_handle)
{
  return;
}

u32
platform_interlocked_increment(volatile u32* value)
{
  return 0;
}

u32
platform_interlocked_cmp_exchg(volatile u32* dest, u32 new_value, u32 old_value)
{
  return 0;
}
