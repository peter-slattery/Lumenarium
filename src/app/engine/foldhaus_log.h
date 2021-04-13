/* date = April 12th 2021 4:25 pm */

#ifndef FOLDHAUS_LOG_H
#define FOLDHAUS_LOG_H

enum log_entry_type
{
    LogEntry_Message,
    LogEntry_Error,
};

struct log_entry
{
    log_entry_type Type;
    gs_string String;
};

struct log_buffer
{
    gs_allocator Allocator;
    
    u64 EntriesCount;
    u64 NextEntry;
    log_entry* Entries;
};

struct log_buffer_iter
{
    log_buffer* Buffer;
    u64 Start;
    u64 IndexAt;
    log_entry* At;
};

internal log_buffer
Log_Init(gs_allocator Allocator, u64 Count)
{
    log_buffer Result = {};
    Result.Allocator = Allocator;
    Result.EntriesCount = Count;
    Result.Entries = AllocatorAllocArray(Allocator, log_entry, Result.EntriesCount);
    
    for (u32 i = 0; i < Result.EntriesCount; i++)
    {
        Result.Entries[i].String = AllocatorAllocString(Allocator, 512);
    }
    
    return Result;
}

internal u64
Log_GetNextIndex(log_buffer Log, u64 At)
{
    u64 Result = At + 1;
    if (Result >= Log.EntriesCount) 
    {
        Result = 0;
    }
    return Result;
}

internal log_entry*
Log_TakeNextEntry(log_buffer* Log)
{
    log_entry* Result = Log->Entries + Log->NextEntry;
    Log->NextEntry = Log_GetNextIndex(*Log, Log->NextEntry);
    return Result;
}

#define Log_Message(log, fmt, ...) Log_PrintF(log, LogEntry_Message, fmt, __VA_ARGS__)
#define Log_Error(log, fmt, ...) Log_PrintF(log, LogEntry_Error, fmt, __VA_ARGS__)
internal void
Log_PrintF(log_buffer* Log, log_entry_type Type, char* Format, ...)
{
    log_entry* NextEntry = Log_TakeNextEntry(Log);
    
    va_list Args;
    va_start(Args, Format);
    NextEntry->String.Length = 0;
    NextEntry->Type = Type;
    PrintFArgsList(&NextEntry->String, Format, Args);
    NullTerminate(&NextEntry->String);
    va_end(Args);
    
#if DEBUG
    OutputDebugStringA(NextEntry->String.Str);
#endif
}

internal log_buffer_iter
Log_GetIter(log_buffer* Buffer)
{
    log_buffer_iter Result = {};
    Result.Buffer = Buffer;
    Result.Start = Buffer->NextEntry;
    Result.IndexAt = Result.Start;
    Result.At = Result.Buffer->Entries + Result.IndexAt;
    return Result;
}

internal bool
LogIter_CanAdvance(log_buffer_iter Iter)
{
    u64 Next = Log_GetNextIndex(*Iter.Buffer, Iter.IndexAt);
    bool Result = Next != Iter.Start;
    return Result;
}

internal void
LogIter_Advance(log_buffer_iter* Iter)
{
    Iter->IndexAt = Log_GetNextIndex(*Iter->Buffer, Iter->IndexAt);
    Iter->At = Iter->Buffer->Entries + Iter->IndexAt;
}

#endif //FOLDHAUS_LOG_H
