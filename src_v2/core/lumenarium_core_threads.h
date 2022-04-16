#if !defined(LUMENARIUM_CORE_THREADS_H)
#define LUMENARIUM_CORE_THREADS_H

typedef struct Thread_Handle Thread_Handle;
struct Thread_Handle { u64 value; };

typedef struct Thread_Result Thread_Result;
struct Thread_Result { u32 code; };

typedef struct Thread_Data Thread_Data;
typedef Thread_Result Thread_Proc(Thread_Data* data);
struct Thread_Data
{
  Thread_Handle thread_handle;
  u32           thread_id;
  Thread_Proc*  thread_proc;
  Allocator*    thread_memory;
  u8*           user_data;
};

#endif // LUMENARIUM_CORE_THREADS_H