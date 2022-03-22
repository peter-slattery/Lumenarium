
#define                     win32_threads_cap   9
global u32                  win32_threads_len = 1;
global HANDLE               win32_threads[win32_threads_cap];
global Platform_Thread_Data win32_threads_data[win32_threads_cap];

DWORD WINAPI
win32_thread_wrapper(void* d)
{
  Platform_Thread_Result result = {};
  Platform_Thread_Data* thread_data = (Platform_Thread_Data*)d;
  thread_data->thread_id = GetCurrentThreadId();
  if (thread_data->thread_proc) 
  {
    result = thread_data->thread_proc(thread_data);
  }
  return result.code;
}

void 
win32_threads_init()
{
  for (u32 i = 1; i < win32_threads_cap; i++) win32_threads[i] = INVALID_HANDLE_VALUE;
}

void
win32_threads_reclaim()
{
  u32 highest_valid = 0;
  for (u32 i = 1; i < win32_threads_cap; i++)
  {
    if (win32_threads[i] != INVALID_HANDLE_VALUE)
    {
      highest_valid = i;
    }
  }
  
  win32_threads_len = highest_valid + 1;
}

Platform_Thread_Handle
platform_thread_begin(Platform_Thread_Proc* proc, u8* user_data)
{
  Platform_Thread_Handle result = {};
  if (win32_threads_len < win32_threads_cap)
  {
    result.value = win32_threads_len++;
  }
  else
  {
    for (u32 i = 1; i < win32_threads_cap; i++)
    {
      if (win32_threads[i] == INVALID_HANDLE_VALUE)
      {
        result.value = i;
        break;
      }
    }
  }
  assert(result.value != 0);
  
  Platform_Thread_Data* thread_data = &win32_threads_data[result.value];
  thread_data->thread_handle = result;
  thread_data->thread_proc = proc;
  thread_data->user_data = user_data;
  
  HANDLE thread_handle = CreateThread(
                                      0, 0, win32_thread_wrapper, (void*)thread_data, 0, 0
                                      );
  win32_threads[result.value] = thread_handle;
  
  return result;
}

void
platform_thread_end(Platform_Thread_Handle thread)
{
  HANDLE thread_handle = win32_threads[thread.value];
  TerminateThread(thread_handle, 0);
  win32_threads[thread.value] = INVALID_HANDLE_VALUE;
}

u32
platform_interlocked_increment(volatile u32* value)
{
  return InterlockedIncrement((LONG volatile*)value);
}

u32
platform_interlocked_cmp_exchg(volatile u32* dest, u32 new_value, u32 old_value)
{
  return InterlockedCompareExchange((LONG volatile*)dest, new_value, old_value);
}
