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
    Result.Allocator = CreateAllocator(Win32Alloc, Win32Free);
    if (Transient != 0)
    {
        Result.Transient = Transient;
    }
    else
    {
        Result.Transient = (gs_memory_arena*)AllocatorAlloc(Result.Allocator, sizeof(gs_memory_arena)).Memory;
        *Result.Transient = CreateMemoryArena(Result.Allocator);
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
#ifdef DEBUG
    // NOTE(Peter): Just prints out the names of all the pending jobs if we end up
    // overflowing the buffer
    if (Queue->JobsCount >= Queue->JobsMax)
    {
        gs_string DebugString = MakeString((char*)malloc(256), 256);
        for (u32 i = 0; i < Queue->JobsCount; i++)
        {
            PrintF(&DebugString, "%d %s\n", i, Queue->Jobs[i].JobName);
            NullTerminate(&DebugString);
            OutputDebugStringA(DebugString.Str);
        }
    }
#endif
    Assert(Queue->JobsCount < Queue->JobsMax);
    
    gs_threaded_job* Job = Queue->Jobs + Queue->JobsCount;
    Job->WorkProc = WorkProc;
    Job->Data = Data;
#ifdef DEBUG
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
        ClearArena(ThreadInfo->ThreadContext.Transient);
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

#define WIN32_FOLDHAUS_WORK_QUEUE_H
#endif // WIN32_FOLDHAUS_WORK_QUEUE_H