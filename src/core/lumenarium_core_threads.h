#if !defined(LUMENARIUM_CORE_THREADS_H)
#define LUMENARIUM_CORE_THREADS_H

typedef struct Thread_Handle Thread_Handle;
struct Thread_Handle { u64 value; };

typedef struct Thread_Result Thread_Result;
struct Thread_Result { u32 code; };

// TODO(PS): In the interest of time while getting Incenter
// up and running, you made Thread_Proc take a u8* rather than
// the whole Thread_Data structure, whcih currently isnt' being
// used at all
typedef struct Thread_Data Thread_Data;
typedef Thread_Result Thread_Proc(u8* data);
struct Thread_Data
{
  Thread_Handle thread_handle;
  u32           thread_id;
  Thread_Proc*  thread_proc;
  Allocator*    thread_memory;
  u8*           user_data;
};



#endif // LUMENARIUM_CORE_THREADS_H