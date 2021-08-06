//
// File: win32_foldhaus_work_queue.h
// Author: Peter Slattery
// Creation Date: 2020-10-01
//
#ifndef WIN32_FOLDHAUS_WORK_QUEUE_H

struct worker_thread_entry
{
  b32 IsValid;
  u32 Index;
};

struct worker_thread_info
{
  gs_thread_context ThreadContext;
  HANDLE Handle;
  gs_work_queue* Queue;
};

struct win32_work_queue
{
  u32 ThreadCount;
  worker_thread_info* Threads;
  gs_work_queue WorkQueue;
};

worker_thread_info* WorkerThreads;
win32_work_queue Win32WorkQueue;

internal s32
Win32GetThreadId()
{
  s32 Result = GetCurrentThreadId();
  return Result;
}

internal gs_thread_context
Win32CreateThreadContext(gs_memory_arena* Transient = 0)
{
  gs_thread_context Result = {0};
  Result.ThreadInfo.ThreadID = Win32GetThreadId();
  Result.Allocator = CreatePlatformAllocator();
  if (Transient != 0)
  {
    Result.Transient = Transient;
  }
  else
  {
    Result.Transient = AllocStruct(Result.Allocator, gs_memory_arena, "Work Queue");
    *Result.Transient = MemoryArenaCreate(MB(4), Bytes(8), Result.Allocator, 0, 0, "Tctx Transient");
  }
  Result.FileHandler = CreateFileHandler(Win32GetFileInfo,
                                         Win32ReadEntireFile,
                                         Win32WriteEntireFile,
                                         Win32EnumerateDirectory,
                                         Result.Transient);
  
  Result.DebugOutput.Print = Win32DebugPrint;
  
  return Result;
}

PUSH_WORK_ON_QUEUE(Win32PushWorkOnQueue)
{
#if DEBUG
  // NOTE(Peter): Just prints out the names of all the pending jobs if we end up
  // overflowing the buffer
  if (Queue->JobsCount >= Queue->JobsMax)
  {
    gs_string DebugString = MakeString((char*)malloc(256), 256);
    for (u32 i = 0; i < Queue->JobsCount; i++)
    {
      Log_Message(GlobalLogBuffer, "%d %s \n", i, Queue->Jobs[i].JobName);
    }
  }
#endif
  Assert(Queue->JobsCount < Queue->JobsMax);
  
  gs_threaded_job* Job = Queue->Jobs + Queue->JobsCount;
  Job->WorkProc = WorkProc;
  Job->Data = Data;
#if DEBUG
  Job->JobName = JobName;
#endif
  
  // Complete Past Writes before Future Writes
  _WriteBarrier();
  _mm_sfence();
  
  ++Queue->JobsCount;
  ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal worker_thread_entry
CompleteAndTakeNextJob(gs_work_queue* Queue, worker_thread_entry Completed, gs_thread_context Context)
{
  if (Completed.IsValid)
  {
    InterlockedIncrement((LONG volatile*)&Queue->JobsCompleted);
  }
  
  worker_thread_entry Result = {};
  Result.IsValid = false;
  
  u32 OriginalNextJobIndex = Queue->NextJobIndex;
  while (OriginalNextJobIndex < Queue->JobsCount)
  {
    u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextJobIndex,
                                           OriginalNextJobIndex + 1,
                                           OriginalNextJobIndex);
    if (Index == OriginalNextJobIndex)
    {
      Result.Index = Index;
      Result.IsValid = true;
      break;
    }
    OriginalNextJobIndex = Queue->NextJobIndex;
  }
  
  return Result;
}

COMPLETE_QUEUE_WORK(Win32DoQueueWorkUntilDone)
{
  worker_thread_entry Entry = {};
  Entry.IsValid = false;
  while (Queue->JobsCompleted < Queue->JobsCount)
  {
    Entry = CompleteAndTakeNextJob(Queue, Entry, Context);
    if (Entry.IsValid)
    {
      Queue->Jobs[Entry.Index].WorkProc(Context, Queue->Jobs[Entry.Index].Data);
    }
  }
}

DWORD WINAPI
WorkerThreadProc (LPVOID InputThreadInfo)
{
  worker_thread_info* ThreadInfo = (worker_thread_info*)InputThreadInfo;
  ThreadInfo->ThreadContext = Win32CreateThreadContext();
  
  worker_thread_entry Entry = {};
  Entry.IsValid = false;
  while (true)
  {
    MemoryArenaClear(ThreadInfo->ThreadContext.Transient);
    Entry = CompleteAndTakeNextJob(ThreadInfo->Queue, Entry, ThreadInfo->ThreadContext);
    if (Entry.IsValid)
    {
      ThreadInfo->Queue->Jobs[Entry.Index].WorkProc(ThreadInfo->ThreadContext,
                                                    ThreadInfo->Queue->Jobs[Entry.Index].Data);
    }
    else
    {
      WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, 0);
    }
  }
  
  return 0;
}

DWORD WINAPI
Win32ThreadProcWrapper(LPVOID ThreadInfo)
{
  platform_thread* Thread = (platform_thread*)ThreadInfo;
  gs_thread_context Ctx = Win32CreateThreadContext();
  Thread->Proc(&Ctx, Thread->UserData);
  
  // TODO(pjs): Destroy Thread Context
  // TODO(pjs): How do we notify the thread manager this thread belongs to that it is free?
  //            Probaby put a pointer to the thread manager in the platform_thread struct
  //            so we can update the tracking structure?
  
  return 0;
}

CREATE_THREAD(Win32CreateThread)
{
  Thread->Proc = Proc;
  Thread->UserData = UserData;
  
  // TODO(pjs): ugh, allocation out in the middle of nowhere
  HANDLE* ThreadHandle = AllocStruct(Ctx.Allocator, HANDLE, "Create Thread");
  *ThreadHandle = CreateThread(0, 0, Win32ThreadProcWrapper, (void*)Thread, 0, 0);
  // TODO(pjs): Error checking on the created thread
  
  Thread->PlatformHandle = (u8*)ThreadHandle;
  
  return true;
}

KILL_THREAD(Win32KillThread)
{
  HANDLE* ThreadHandle = (HANDLE*)Thread->PlatformHandle;
  TerminateThread(ThreadHandle, 0);
  
  // TODO(pjs): see allocation out in the middle of nowhere in Win32CreateThread
  Win32Free((void*)Thread->PlatformHandle, sizeof(HANDLE), 0);
  
  // TODO(pjs): Error checking
  return true;
}

internal void
Win32WorkQueue_Init(gs_memory_arena* Arena, u32 ThreadCount)
{
  if (ThreadCount > 0)
  {
    Win32WorkQueue.ThreadCount = ThreadCount;
    Win32WorkQueue.Threads = PushArray(Arena, worker_thread_info, ThreadCount);
  }
  
  gs_work_queue WQ = {};
  WQ.SemaphoreHandle = CreateSemaphoreEx(0, 0, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);;
  WQ.JobsMax = 512;
  WQ.Jobs = PushArray(Arena, gs_threaded_job, WQ.JobsMax);
  WQ.NextJobIndex = 0;
  WQ.PushWorkOnQueue = Win32PushWorkOnQueue;
  WQ.CompleteQueueWork = Win32DoQueueWorkUntilDone;
  
  Win32WorkQueue.WorkQueue = WQ;
  
  // ID = 0 is reserved for this thread
  for (u32 i = 0; i < ThreadCount; i++)
  {
    worker_thread_info* T = Win32WorkQueue.Threads + i;
    T->Queue = &Win32WorkQueue.WorkQueue;
    T->Handle = CreateThread(0, 0, &WorkerThreadProc, (void*)T, 0, 0);
  }
}

internal void
Win32WorkQueue_Cleanup()
{
  u32 Error = 0;
  for (u32 Thread = 0; Thread < Win32WorkQueue.ThreadCount; Thread++)
  {
    u32 Success = TerminateThread(Win32WorkQueue.Threads[Thread].Handle, 0);
    if (!Success)
    {
      Error = GetLastError();
      InvalidCodePath;
    }
  }
}

#define WIN32_FOLDHAUS_WORK_QUEUE_H
#endif // WIN32_FOLDHAUS_WORK_QUEUE_H