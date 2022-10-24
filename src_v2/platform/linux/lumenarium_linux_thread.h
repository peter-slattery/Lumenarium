#ifndef LUMENARIUM_RASPI_THREADS_H
#define LUMENARIUM_RASPI_THREADS_H 1

typedef struct Os_Linux_Thread Os_Linux_Thread;
struct Os_Linux_Thread
{
  pthread_t thread;
  Thread_Proc* proc;
  u8* user_data;
  Thread_Result result;
};

#define LINUX_THREAD_CAP 8
global Os_Linux_Thread linux_threads_[LINUX_THREAD_CAP];
global u32             linux_threads_len_;

Thread_Handle 
os_thread_begin(Thread_Proc* proc, u8* user_data)
{
  // Call the actual thread function
  // This is essentially a blocking call for the rest of this function
  Os_Linux_Thread* thread_data = (Os_Linux_Thread*)arg;
  thread_data->result = thread_data->proc(thread_data->user_data);
  
  // Clean up this threads thread slot so other threads
  // can use it
  thread_data->proc = 0;
  thread_data->user_data = 0;
  
  pthread_exit(&thread_data->result);
}

void          
os_thread_end(Thread_Handle thread_handle)
{
  // Find an unused thread slot
  Thread_Handle result = { .value = 0 };
  if (linux_threads_len_ < OSX_THREAD_CAP) 
  {
    result = (Thread_Handle){ .value = linux_threads_len_++ };
  }
  else
  {
    bool found = false;
    for (u32 i = 0; i < OSX_THREAD_CAP; i++)
    {
      if (linux_threads_[i].proc == 0) 
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
  Os_Linux_Thread* t = linux_threads_ + result.value;  
  *t = (Os_Linux_Thread){
    .proc = proc,
    .user_data = user_data
  };
  
  // Create PThread
  s32 create_error = pthread_create(&t->thread, 
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
  Os_Linux_Thread* t = linux_threads_ + thread_handle.value;
  pthread_kill(t->thread, 0);
}

u32 
os_interlocked_increment(volatile u32* value)
{
  invalid_code_path;
  return 0;
}

bool
os_interlocked_cmp_exchg(volatile u32* dest, u32 old_value, u32 new_value)
{
  invalid_code_path;
  return 0;
}


#endif // LUMENARIUM_RASPI_THREADS_H