//
// File: foldhaus_debug.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
// DESCRIPTION:
// This file contains profiling capabilities for both the engine and app
//
#ifndef FOLDHAUS_DEBUG_H

#define SCOPE_NAME_LENGTH 256
struct scope_record
{
    u32 NameHash;
    s64 StartCycles;
    s64 EndCycles;
    s32 CallDepth;
};

struct collated_scope_record
{
    u32 NameHash;
    s64 TotalCycles;
    s32 CallCount;
    
    r32 PercentFrameTime;
    r32 TotalSeconds;
    
    r32 AverageSecondsPerCall;
};

#define SCOPE_NAME_BUFFER_LENGTH 128
struct scope_name
{
    u32 Hash;
    gs_string Name;
    char Buffer[SCOPE_NAME_BUFFER_LENGTH];
};

struct debug_scope_record_list
{
    s32 ThreadId;
    s32 Max;
    s32 Count;
    scope_record* Calls;
    
    s32 CurrentScopeCallDepth;
};

#define DEBUG_FRAME_GROW_SIZE 8102
struct debug_frame
{
    s64 FrameStartCycles;
    s64 FrameEndCycles;
    
    s32 ScopeNamesMax;
    s32 ScopeNamesCount;
    scope_name* ScopeNamesHash;
    
    s32 ThreadCount;
    debug_scope_record_list* ThreadCalls;
    
    s32 CollatedScopesMax;
    collated_scope_record* CollatedScopes;
};

enum debug_ui_view
{
    DebugUI_Profiler,
    DebugUI_ScopeList,
    DebugUI_MemoryView,
    
    DebugUI_Count,
};

struct debug_interface
{
    b32 ShowCameraMouse;
    b32 ShowTrackedScopes;
    b32 RenderSculpture;
    b32 SendSACNData;
    
    s32 FrameView;
};

typedef s32 debug_get_thread_id();
typedef s64 debug_timing_proc();
typedef u8* debug_alloc(s32 ElementSize, s32 ElementCount);
typedef u8* debug_realloc(u8* Memory, s32 OldSize, s32 NewSize);

#define HISTOGRAM_DEPTH 10
struct debug_histogram_entry
{
    char ScopeName_[SCOPE_NAME_LENGTH];
    gs_string ScopeName;
    
    u32 PerFrame_Cycles[HISTOGRAM_DEPTH];
    u32 PerFrame_CallCount[HISTOGRAM_DEPTH];
    s32 CurrentFrame;
    
    // NOTE(Peter): Cached Values, recalculated ever frame
    u32 Average_Cycles;
    u32 Average_CallCount;
    u32 Total_Cycles;
    u32 Total_CallCount;
};

#define DEBUG_FRAME_COUNT 128
struct debug_services
{
    s64 PerformanceCountFrequency;
    
    b32 RecordFrames;
    s32 CurrentDebugFrame;
    debug_frame Frames[DEBUG_FRAME_COUNT];
    
    debug_interface Interface;
    
    debug_get_thread_id* GetThreadId;
    debug_timing_proc* GetWallClock;
    debug_alloc* Alloc;
    debug_realloc* Realloc;
};

internal void
InitializeDebugFrame (debug_frame* Frame, s32 NameHashMax, s32 ThreadCount, s32 ScopeCallsMax, debug_services* Services)
{
    Frame->ScopeNamesMax = NameHashMax;
    Frame->ScopeNamesHash = (scope_name*)Services->Alloc(sizeof(scope_name), NameHashMax);
    
    // NOTE(Peter): We use the same size as scope names because we're only storing a single instance
    // per scope. If ScopeNamesMax can't hold all the scopes, this will never get filled and
    // we should assert and recompile with a resized NameHashMax
    Frame->CollatedScopesMax = NameHashMax;
    Frame->CollatedScopes = (collated_scope_record*)Services->Alloc(sizeof(collated_scope_record), NameHashMax);
    
    for (s32 i = 0; i < Frame->ScopeNamesMax; i++)
    {
        scope_name* Entry = Frame->ScopeNamesHash + i;
        Entry->Name = MakeString(Entry->Buffer, 0, SCOPE_NAME_BUFFER_LENGTH);
    }
    
    Frame->ThreadCount = ThreadCount;
    Frame->ThreadCalls = (debug_scope_record_list*)Services->Alloc(sizeof(debug_scope_record_list),
                                                                   ThreadCount);
    
    for (s32 i = 0; i < ThreadCount; i++)
    {
        Frame->ThreadCalls[i].Max = ScopeCallsMax;
        Frame->ThreadCalls[i].Count = 0;
        Frame->ThreadCalls[i].Calls = (scope_record*)Services->Alloc(sizeof(scope_record), ScopeCallsMax);
        Frame->ThreadCalls[i].CurrentScopeCallDepth = 0;
        Frame->ThreadCalls[i].ThreadId = 0;
    }
    
    for (s32 c = 0; c < Frame->CollatedScopesMax; c++)
    {
        Frame->CollatedScopes[c].NameHash = 0;
    }
}

internal void
StartDebugFrame(debug_frame* Frame, debug_services* Services)
{
    Frame->FrameStartCycles = Services->GetWallClock();
    for (s32 i = 0; i < Frame->ThreadCount; i++)
    {
        Frame->ThreadCalls[i].Count = 0;
        Frame->ThreadCalls[i].CurrentScopeCallDepth = 0;
    }
    
    for (s32 c = 0; c < Frame->CollatedScopesMax; c++)
    {
        s32 Hash = Frame->CollatedScopes[c].NameHash;
        Frame->CollatedScopes[c] = {};
        Frame->CollatedScopes[c].NameHash = Hash;
    }
}

internal void
InitDebugServices (debug_services* Services,
                   s64 PerformanceCountFrequency,
                   debug_alloc* Alloc,
                   debug_realloc* Realloc,
                   debug_timing_proc* GetWallClock,
                   debug_get_thread_id* GetThreadId,
                   s32 ThreadCount)
{
    Services->Alloc = Alloc;
    Services->Realloc = Realloc;
    Services->GetWallClock = GetWallClock;
    Services->GetThreadId = GetThreadId;
    
    Services->RecordFrames = true;
    
    Services->CurrentDebugFrame = 0;
    s32 NameHashMax = 4096;
    s32 ScopeCallsMax = 4096;
    for (s32 i = 0; i < DEBUG_FRAME_COUNT; i++)
    {
        InitializeDebugFrame(&Services->Frames[i], NameHashMax, ThreadCount, ScopeCallsMax,  Services);
    }
    
    Services->Interface.RenderSculpture = true;
    
    Services->PerformanceCountFrequency = PerformanceCountFrequency;
    
    Services->Interface.ShowCameraMouse = false;
    Services->Interface.ShowTrackedScopes = false;
    Services->Interface.RenderSculpture = true;
    Services->Interface.SendSACNData = false;
}

internal debug_frame*
GetCurrentDebugFrame (debug_services* Services)
{
    debug_frame* Result = Services->Frames + Services->CurrentDebugFrame;
    return Result;
}

internal debug_frame*
GetLastDebugFrame(debug_services* Services)
{
    s32 Index = (Services->CurrentDebugFrame - 1);
    if (Index < 0) { Index += DEBUG_FRAME_COUNT; }
    debug_frame* Result = Services->Frames + Index;
    return Result;
}

internal s32
GetIndexForNameHash(debug_frame* Frame, u32 NameHash)
{
    s32 Result = -1;
    
    for (s32 Offset = 0; Offset < Frame->ScopeNamesMax; Offset++)
    {
        u32 Index = (NameHash + Offset) % Frame->ScopeNamesMax;
        if (Frame->ScopeNamesHash[Index].Hash == NameHash)
        {
            Result = Index;
            break;
        }
    }
    
    // NOTE(Peter): Its not technically wrong to return a -1 here, just means we didn't find it.
    // At the time of writing however, this function is only being called in contexts where we
    // know there should be an entry in the Name table, so a -1 actually indicates a problem.
    Assert(Result >= 0);
    return Result;
}

internal debug_scope_record_list*
GetScopeListForThreadInFrame(debug_services* Services, debug_frame* Frame)
{
    debug_scope_record_list* List = 0;
    
    s32 CurrentThreadId = Services->GetThreadId();
    for (s32 Offset = 0; Offset < Frame->ThreadCount; Offset++)
    {
        s32 Index = (CurrentThreadId + Offset) % Frame->ThreadCount;
        if (Frame->ThreadCalls[Index].ThreadId == CurrentThreadId)
        {
            List = Frame->ThreadCalls + Index;
            break;
        }
        else if (Frame->ThreadCalls[Index].ThreadId == 0)
        {
            Frame->ThreadCalls[Index].ThreadId = CurrentThreadId;
            List = Frame->ThreadCalls + Index;
            break;
        }
    }
    
    Assert(List);
    return List;
}

internal void
CollateThreadScopeCalls (debug_scope_record_list* ThreadRecords, debug_frame* Frame)
{
    for (s32 i = 0; i < ThreadRecords->Count; i++)
    {
        scope_record Record = ThreadRecords->Calls[i];
        s32 Index = GetIndexForNameHash (Frame, Record.NameHash);
        collated_scope_record* CollatedRecord = Frame->CollatedScopes + Index;
        
        if (CollatedRecord->NameHash != Record.NameHash)
        {
            CollatedRecord->NameHash = Record.NameHash;
            CollatedRecord->TotalCycles = 0;
            CollatedRecord->CallCount = 0;
        }
        
        CollatedRecord->TotalCycles += Record.EndCycles - Record.StartCycles;
        CollatedRecord->CallCount += 1;
    }
}

internal void
EndDebugFrame (debug_services* Services)
{
    debug_frame* ClosingFrame = GetCurrentDebugFrame(Services);
    ClosingFrame->FrameEndCycles = Services->GetWallClock();
    
    s64 FrameTotalCycles = ClosingFrame->FrameEndCycles - ClosingFrame->FrameStartCycles;
    
    for (s32 t = 0; t < ClosingFrame->ThreadCount; t++)
    {
        CollateThreadScopeCalls(ClosingFrame->ThreadCalls + t, ClosingFrame);
    }
    
    s32 ScopeNamesCount = 0;
    for (s32 n = 0; n < ClosingFrame->ScopeNamesMax; n++)
    {
        if (ClosingFrame->ScopeNamesHash[n].Hash != 0)
        {
            collated_scope_record* CollatedRecord = ClosingFrame->CollatedScopes + n;
            CollatedRecord->TotalSeconds = (r32)CollatedRecord->TotalCycles / (r32)Services->PerformanceCountFrequency;
            CollatedRecord->PercentFrameTime = (r32)CollatedRecord->TotalCycles / (r32)FrameTotalCycles;
            CollatedRecord->AverageSecondsPerCall = CollatedRecord->TotalSeconds / CollatedRecord->CallCount;
            ScopeNamesCount += 1;
        }
    }
    ClosingFrame->ScopeNamesCount = ScopeNamesCount;
    
    Services->CurrentDebugFrame = (Services->CurrentDebugFrame + 1) % DEBUG_FRAME_COUNT;
    StartDebugFrame(&Services->Frames[Services->CurrentDebugFrame], Services);
}

internal u32
HashScopeName (char* ScopeName)
{
    // djb2 hash
    u32 Hash = 5381;
    char* C = ScopeName;
    while(*C)
    {
        Hash = ((Hash << 5) + Hash) + *C;
        C++;
    }
    return Hash;
}

internal scope_name*
GetOrAddNameHashEntry(debug_frame* Frame, u32 NameHash)
{
    scope_name* Result = 0;
    
    for (s32 Offset = 0; Offset < Frame->ScopeNamesMax; Offset++)
    {
        u32 Index = (NameHash + Offset) % Frame->ScopeNamesMax;
        if ((Frame->ScopeNamesHash[Index].Hash == 0) || (Frame->ScopeNamesHash[Index].Hash == NameHash))
        {
            Result = Frame->ScopeNamesHash + Index;
            break;
        }
    }
    
    return Result;
}

internal u32
BeginTrackingScopeAndGetNameHash (debug_services* Services, char* ScopeName)
{
    debug_frame* CurrentFrame = GetCurrentDebugFrame(Services);
    debug_scope_record_list* ThreadList = GetScopeListForThreadInFrame(Services, CurrentFrame);
    
    ThreadList->CurrentScopeCallDepth++;
    
    u32 NameHash = HashScopeName(ScopeName);
    scope_name* Entry = GetOrAddNameHashEntry(CurrentFrame, NameHash);
    if (Entry->Hash == 0) // If its new
    {
        Entry->Hash = NameHash;
        // TODO(Peter): need to initialize all entry name gs_strings to point at the buffer
        // This will break eventually. when it does, do this ^^^^ when on startup
        PrintF(&Entry->Name, "%s", ScopeName);
    }
    
    return NameHash;
}

internal void
PushScopeTimeOnFrame (debug_services* Services, s32 NameHash, u64 StartCycles, u64 EndCycles)
{
    debug_frame* CurrentFrame = GetCurrentDebugFrame(Services);
    debug_scope_record_list* ThreadList = GetScopeListForThreadInFrame(Services, CurrentFrame);
    
    if (ThreadList->Count >= ThreadList->Max)
    {
        s32 CurrentSize = ThreadList->Max * sizeof(scope_record);
        s32 NewMax = (ThreadList->Max + DEBUG_FRAME_GROW_SIZE);
        s32 NewSize = NewMax * sizeof(scope_record);
        ThreadList->Calls = (scope_record*)Services->Realloc((u8*)ThreadList->Calls, CurrentSize, NewSize);
        ThreadList->Max = NewMax;
    }
    
    Assert(ThreadList->Count < ThreadList->Max);
    
    s32 EntryIndex = ThreadList->Count++;
    scope_record* Record = ThreadList->Calls + EntryIndex;
    Record->NameHash = NameHash;
    Record->StartCycles = StartCycles;
    Record->EndCycles = EndCycles;
    Record->CallDepth = --ThreadList->CurrentScopeCallDepth;
}

internal r32 DEBUGGetSecondsElapsed (s64 Start, s64 End, r32 PerformanceCountFrequency)
{
    r32 Result = ((r32)(End - Start) / (r32)PerformanceCountFrequency);
    return Result;
}

#ifdef DEBUG
#define DEBUG_TRACK_FUNCTION scope_tracker ScopeTracker ((char*)__func__, GlobalDebugServices)
#define DEBUG_TRACK_SCOPE(name) scope_tracker ScopeTracker_##name (#name, GlobalDebugServices)
#else
#define DEBUG_TRACK_FUNCTION
#define DEBUG_TRACK_SCOPE(name)
#endif
struct scope_tracker
{
    s64 ScopeStart;
    u32 ScopeNameHash;
    debug_services* DebugServices;
    
    scope_tracker(char* ScopeName, debug_services* DebugServices)
    {
        if (DebugServices->RecordFrames)
        {
            this->ScopeNameHash = BeginTrackingScopeAndGetNameHash(DebugServices, ScopeName);
            this->ScopeStart = DebugServices->GetWallClock();
            this->DebugServices = DebugServices;
        }
        else
        {
            this->DebugServices = 0;
        }
    }
    
    ~scope_tracker()
    {
        if (this->DebugServices) // NOTE(Peter): If DebugServices == 0, then we werent' recording this frame
        {
            s64 ScopeEnd = this->DebugServices->GetWallClock();
            PushScopeTimeOnFrame(this->DebugServices, this->ScopeNameHash, this->ScopeStart, ScopeEnd);
        }
    }
};


#define FOLDHAUS_DEBUG_H
#endif // FOLDHAUS_DEBUG_H