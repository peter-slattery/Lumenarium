/* date = August 13th 2022 1:57 pm */

#ifndef LUMENARIUM_OSX_THREAD_H
#define LUMENARIUM_OSX_THREAD_H

typedef struct Os_Osx_Thread Os_Osx_Thread;
struct Os_Osx_Thread
{
  pthread_t thread;
  Thread_Proc* proc;
  u8* user_data;
  Thread_Result result;
};

#define OSX_THREAD_CAP 8
global Os_Osx_Thread osx_threads_[OSX_THREAD_CAP];
global u32           osx_threads_len_;

void*
os_thread_proc_wrapper(void* arg)
{
  // Call the actual thread function
  // This is essentially a blocking call for the rest of this function
  Os_Osx_Thread* thread_data = (Os_Osx_Thread*)arg;
  thread_data->result = thread_data->proc(thread_data->user_data);
  
  // Clean up this threads thread slot so other threads
  // can use it
  thread_data->proc = 0;
  thread_data->user_data = 0;
  
  pthread_exit(&thread_data->result);
}

Thread_Handle 
os_thread_begin(Thread_Proc* proc, u8* user_data)
{
  // Find an unused thread slot
  Thread_Handle result = { .value = 0 };
  if (osx_threads_len_ < OSX_THREAD_CAP) 
  {
    result = (Thread_Handle){ .value = osx_threads_len_++ };
  }
  else
  {
    bool found = false;
    for (u32 i = 0; i < OSX_THREAD_CAP; i++)
    {
      if (osx_threads_[i].proc == 0) 
      {
        result.value = i;
        found = true;
      }
    }
    
    if (!found) {
      printf("ERROR: Unable to create thread.\n");
      invalid_code_path;
    }
  }
  
  // Initialize Thread Slot
  Os_Osx_Thread* t = osx_threads_ + result.value;  
  *t = (Os_Osx_Thread){
    .proc = proc,
    .user_data = user_data
  };
  
  // Create PThread
  s32 create_error = pthread_create(
      &t->thread, 
    NULL,  // use default attrs
    (void * _Nullable (* _Nonnull)(void * _Nullable))proc, 
    user_data
  );
  
  if (create_error) 
  {
    switch (create_error) {
      case EAGAIN: {
      } break;
      
      case EINVAL: {
      } break;
      
      //      case ELEMULTITHREADFORK: {
      //      } break;
      
      case ENOMEM: {
      } break;
    }
  }
  
  return result;
}

void          
os_thread_end(Thread_Handle thread_handle)
{
  Os_Osx_Thread* t = osx_threads_ + thread_handle.value;
  pthread_kill(t->thread, 0);
}

u32 
os_interlocked_increment(volatile u32* value)
{
  assert(*value <= s32_max);
  u32 result = OSAtomicIncrement32((volatile s32*)value);
  return result;
}

bool
os_interlocked_cmp_exchg(volatile u32* dest, u32 old_value, u32 new_value)
{
  assert(*dest <= s32_max);
  bool result = OSAtomicCompareAndSwapInt((s32)old_value, (s32)new_value, (volatile s32*)dest);
  return result;
}

#endif //LUMENARIUM_OSX_THREAD_H
