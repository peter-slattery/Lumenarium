#define SCOPE_NAME_LENGTH 256
struct scope_time_record
{
    char ScopeName_[SCOPE_NAME_LENGTH];
    string ScopeName;
    
    u32 Duration_Cycles;
};

struct debug_interface
{
    b32 ShowCameraMouse;
    b32 ShowTrackedScopes;
    b32 RenderSculpture;
    b32 SendSACNData;
};

typedef s64 debug_timing_proc();

#define HISTOGRAM_DEPTH 10
struct debug_histogram_entry
{
    char ScopeName_[SCOPE_NAME_LENGTH];
    string ScopeName;
    
    u32 PerFrame_Cycles[HISTOGRAM_DEPTH];
    u32 PerFrame_CallCount[HISTOGRAM_DEPTH];
    s32 CurrentFrame;
    
    // NOTE(Peter): Cached Values, recalculated ever frame
    u32 Average_Cycles;
    u32 Average_CallCount;
    u32 Total_Cycles;
    u32 Total_CallCount;
};

#define SCOPE_HISTOGRAM_SIZE 512
struct debug_services
{
    s32 TrackedScopesCount;
    s32 TrackedScopesMax;
    scope_time_record* TrackedScopes;
    
    s64 PerformanceCountFrequency;
    memory_arena DebugStorage;
    
    debug_interface Interface;
    
    debug_timing_proc* GetWallClock;
    
    debug_histogram_entry ScopeHistogram[SCOPE_HISTOGRAM_SIZE];
    s32 ScopeHistogramUsed;
};

internal void
InitDebugServices (debug_services* Services, u8* Memory, s32 MemorySize, s32 TrackedScopesMax, s64 PerformanceCountFrequency)
{
    InitMemoryArena(&Services->DebugStorage, Memory, MemorySize, 0);
    
    Services->TrackedScopesCount = 0;
    Services->TrackedScopesMax = TrackedScopesMax;
    Services->TrackedScopes = PushArray(&Services->DebugStorage, scope_time_record, TrackedScopesMax);
    
    Services->Interface.RenderSculpture = true;
    
    Services->PerformanceCountFrequency = PerformanceCountFrequency;
    
    Services->Interface.ShowCameraMouse = false;
    Services->Interface.ShowTrackedScopes = false;
    Services->Interface.RenderSculpture = true;
    Services->Interface.SendSACNData = false;
    
    Services->ScopeHistogramUsed = 0;
}

internal s32
DEBUGFindScopeHistogram (debug_services* Services, string Name)
{
    s32 Result = -1;
    for (s32 i = 0; i < SCOPE_HISTOGRAM_SIZE; i++)
    {
        if (StringsEqual(Services->ScopeHistogram[i].ScopeName, Name))
        {
            Result = i;
            break;
        }
    }
    return Result;
}

internal s32
DEBUGAddScopeHistogram (debug_services* Services, scope_time_record Record)
{
    Assert(Services->ScopeHistogramUsed < SCOPE_HISTOGRAM_SIZE);
    
    s32 Result = Services->ScopeHistogramUsed++;
    
    debug_histogram_entry* Entry = Services->ScopeHistogram + Result;
    Entry->ScopeName = MakeString(Entry->ScopeName_, 256);
    
    Entry->CurrentFrame = 0;
    Entry->Average_Cycles = 0;
    Entry->Average_CallCount = 0;
    Entry->Total_Cycles = 0;
    Entry->Total_CallCount = 0;
    
    CopyStringTo(Record.ScopeName, &Entry->ScopeName);
    
    return Result;
}

internal void
DEBUGRecordScopeInHistogram (debug_services* Services, s32 Index, scope_time_record Record)
{
    debug_histogram_entry* Entry = Services->ScopeHistogram + Index;
    s32 FrameIndex = Entry->CurrentFrame;
    if (FrameIndex >= 0 && FrameIndex < HISTOGRAM_DEPTH)
    {
        Entry->PerFrame_Cycles[FrameIndex] += Record.Duration_Cycles;
        Entry->PerFrame_CallCount[FrameIndex]++;
    }
}

internal void
DEBUGCacheScopeHistogramValues (debug_histogram_entry* Histogram)
{
    Histogram->Total_Cycles = 0;
    Histogram->Total_CallCount = 0;
    
    // TODO(Peter): This doesn't account for the first frames when the histogram isn't full
    for (s32 i = 0; i < HISTOGRAM_DEPTH; i++)
    {
        Histogram->Total_Cycles += Histogram->PerFrame_Cycles[i];
        Histogram->Total_CallCount += Histogram->PerFrame_CallCount[i];
    }
    
    Histogram->Average_Cycles = (Histogram->Total_Cycles / HISTOGRAM_DEPTH);
    Histogram->Average_CallCount = (Histogram->Total_CallCount / HISTOGRAM_DEPTH);
}

internal void
DEBUGCollateScopeRecords (debug_services* Services)
{
    for (s32 i = 0; i < Services->TrackedScopesCount; i++)
    {
        scope_time_record Record = Services->TrackedScopes[i];
        s32 Index = DEBUGFindScopeHistogram(Services, Record.ScopeName);
        if (Index < 0)
        {
            Index = DEBUGAddScopeHistogram(Services, Record);
        }
        
        DEBUGRecordScopeInHistogram(Services, Index, Record);
    }
    
    for (s32 h = 0; h < Services->ScopeHistogramUsed; h++)
    {
        DEBUGCacheScopeHistogramValues(Services->ScopeHistogram + h);
    }
}

internal void
EndDebugFrame (debug_services* Services)
{
    DEBUGCollateScopeRecords(Services);
    
    GSZeroMemory((u8*)Services->TrackedScopes, sizeof(scope_time_record) * Services->TrackedScopesMax);
    Services->TrackedScopesCount = 0;
    
    for (s32 i = 0; i < Services->ScopeHistogramUsed; i++)
    {
        s32 NewFrame = Services->ScopeHistogram[i].CurrentFrame + 1;
        if (NewFrame >= HISTOGRAM_DEPTH)
        {
            NewFrame = 0;
        }
        Services->ScopeHistogram[i].CurrentFrame = NewFrame;
        Services->ScopeHistogram[i].PerFrame_Cycles[NewFrame] = 0;
        Services->ScopeHistogram[i].PerFrame_CallCount[NewFrame] = 0;
    }
}

internal scope_time_record*
PushScopeRecord(string ScopeName, debug_services* Services)
{
    scope_time_record* Result = 0;
    
    s32 OnePastIndex = InterlockedIncrement((long*)&Services->TrackedScopesCount);
    Assert(OnePastIndex <= Services->TrackedScopesMax);
    Result = Services->TrackedScopes + OnePastIndex - 1;
    
    Result->ScopeName = MakeString(Result->ScopeName_, SCOPE_NAME_LENGTH);
    CopyStringTo(ScopeName, &Result->ScopeName);
    
    return Result;
}

internal void
LogScopeTime (debug_services* Services, string ScopeName, u64 CyclesElapsed)
{
    scope_time_record* Record = PushScopeRecord(ScopeName, Services);
    Record->Duration_Cycles = CyclesElapsed;
}

internal r32 DEBUGGetSecondsElapsed (s64 Start, s64 End, r32 PerformanceCountFrequency)
{
    r32 Result = ((r32)(End - Start) / (r32)PerformanceCountFrequency);
    return Result;
}

#if 1
#define DEBUG_TRACK_FUNCTION scope_tracker ScopeTracker (__FUNCTION__, GlobalDebugServices)
#define DEBUG_TRACK_SCOPE(name) scope_tracker ScopeTracker_##name (#name, GlobalDebugServices)
#else
#define DEBUG_TRACK_FUNCTION 
#define DEBUG_TRACK_SCOPE(name)
#endif
struct scope_tracker
{
    s64 ScopeStart;
    
    char ScopeName_[SCOPE_NAME_LENGTH];
    string ScopeName; 
    
    debug_services* DebugServices;
    
    scope_tracker(char* ScopeName, debug_services* DebugServices)
    {
        this->ScopeName = MakeString(this->ScopeName_, SCOPE_NAME_LENGTH);
        CopyCharArrayToString(ScopeName, &this->ScopeName);
        this->ScopeStart = DebugServices->GetWallClock();
        this->DebugServices = DebugServices;
    }
    
    ~scope_tracker()
    {
        s64 ScopeEnd = DebugServices->GetWallClock();
        u32 CyclesElapsed = (u32)(ScopeEnd - this->ScopeStart);
#if 0
        r32 SecondsElapsed = DEBUGGetSecondsElapsed(this->ScopeStart, ScopeEnd, DebugServices->PerformanceCountFrequency);
#endif
        LogScopeTime(DebugServices, ScopeName, CyclesElapsed);
    }
};
