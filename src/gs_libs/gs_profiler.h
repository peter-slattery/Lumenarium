//
// File: gs_profiler.h
// Author: Peter Slattery
// Creation Date: 2020-05-11
//
#ifndef GS_PROFILER_H

internal s64
GetWallClock()
{
    LARGE_INTEGER Time;
    if (!QueryPerformanceCounter(&Time))
    {
        s32 Error = GetLastError();
        // TODO(Peter): I'm waiting to see an error actually occur here
        // to know what it could possibly be.
        InvalidCodePath;
    }
    return (s64)Time.QuadPart;
}

internal u64
HashDJB2ToU64(char* String)
{
    u64 Hash = 5381;
    char* C = String;
    while(*C)
    {
        Hash = ((Hash << 5) + Hash) + *C++;
    }
    return Hash;
}

struct scope_record
{
    u64 IdentifierHash;
    s64 StartTime;
    s64 EndTime;
};

struct scope_record_list
{
    scope_record* List;
    u32 Count;
    u32 MaxCount;
    scope_record_list* Next;
};

struct collated_scope_record
{
    char* Identifier;
    u64 CallCount;
    u64 TotalCycles;
    u64 LongestDuration;
};

#define COLLATED_SCOPES_HASH_SIZE 1024
struct collated_scopes_hash_table
{
    u64* Hashes;
    collated_scope_record* Scopes;
    collated_scopes_hash_table* Next;
};

struct debug_services
{
    memory_arena Arena;
    memory_arena Transient;
    bool ShouldProfile;
    
    u32 CollatedScopesCount;
    collated_scopes_hash_table* CollatedScopes;
    
    scope_record_list* ScopeRecordsHead;
    scope_record_list* ScopeRecordsTail;
};

global_variable debug_services GlobalDebugServices = {0};

internal u64
RegisterScope(debug_services* Services, char* Identifier)
{
    u64 Hash = HashDJB2ToU64(Identifier);
    u32 Index = (u32)(Hash % COLLATED_SCOPES_HASH_SIZE);
    
    collated_scopes_hash_table* PrevTable = 0;
    collated_scopes_hash_table* TableAt = Services->CollatedScopes;
    while (TableAt != 0)
    {
        if (TableAt->Hashes[Index] == Hash || TableAt->Hashes[Index] == 0)
        {
            break;
        }
        PrevTable = TableAt;
        TableAt = TableAt->Next;
    }
    
    if (!TableAt)
    {
        collated_scopes_hash_table* NewTable = PushStruct(&Services->Arena, collated_scopes_hash_table);
        NewTable->Hashes = PushArray(&Services->Arena, u64, COLLATED_SCOPES_HASH_SIZE);
        GSZeroMemory(NewTable->Hashes, sizeof(u64) * COLLATED_SCOPES_HASH_SIZE);
        NewTable->Scopes = PushArray(&Services->Arena, collated_scope_record, COLLATED_SCOPES_HASH_SIZE);
        NewTable->Next = 0;
        
        if (PrevTable)
        {
            PrevTable->Next = NewTable;
        }
        else
        {
            Services->CollatedScopes = NewTable;
        }
        
        TableAt = NewTable;
    }
    
    if (TableAt->Hashes[Index] == 0)
    {
        // New Scope Being Registered
        TableAt->Hashes[Index] = Hash;
        TableAt->Scopes[Index] = {0};
        TableAt->Scopes[Index].Identifier = Identifier;
        Services->CollatedScopesCount++;
    }
    else if (TableAt->Hashes[Index] != Hash)
    {
        InvalidCodePath;
    }
    
    return Hash;
}

internal void
PushScopeOnList(debug_services* Services, char* ScopeName, s64 Start, s64 End)
{
    if (!Services->ScopeRecordsTail ||
        Services->ScopeRecordsTail->Count >= Services->ScopeRecordsTail->MaxCount)
    {
        scope_record_list* NewTail = PushStruct(&Services->Arena, scope_record_list);
        NewTail->MaxCount = 4096;
        NewTail->List = PushArray(&Services->Arena, scope_record, NewTail->MaxCount);
        NewTail->Count = 0;
        NewTail->Next = 0;
        
        if (Services->ScopeRecordsHead)
        {
            Services->ScopeRecordsTail->Next = NewTail;
        }
        else
        {
            Services->ScopeRecordsHead = NewTail;
        }
        Services->ScopeRecordsTail = NewTail;
    }
    
    u32 NewRecordIndex = Services->ScopeRecordsTail->Count++;
    
    scope_record* NewRecord = &Services->ScopeRecordsTail->List[NewRecordIndex];
    NewRecord->IdentifierHash = RegisterScope(Services, ScopeName);
    NewRecord->StartTime = Start;
    NewRecord->EndTime = End;
}

#define DEBUG_TRACK_FUNCTION scope_tracker ScopeTracker ((char*)__func__, &GlobalDebugServices)
struct scope_tracker
{
    s64 ScopeStart;
    char* ScopeName;
    debug_services* Services;
    scope_tracker(char* ScopeName, debug_services* Services)
    {
        if (Services && Services->ShouldProfile)
        {
            this->ScopeName = ScopeName;
            this->ScopeStart = GetWallClock();
            this->Services = Services;
        }
        else
        {
            this->Services = 0;
        }
    }
    
    ~scope_tracker()
    {
        if (this->Services != 0)
        {
            s64 ScopeEnd = GetWallClock();
            PushScopeOnList(&GlobalDebugServices, this->ScopeName, this->ScopeStart, ScopeEnd);
        }
    }
};

//
// Reporting
//

internal collated_scope_record*
GetCollatedRecordForScope(debug_services* Services, u64 Hash)
{
    Assert(Services->CollatedScopes);
    collated_scope_record* Result = 0;
    u64 Index = Hash % COLLATED_SCOPES_HASH_SIZE;
    
    collated_scopes_hash_table* TableAt = Services->CollatedScopes;
    while (TableAt != 0)
    {
        if (TableAt->Hashes[Index] == Hash)
        {
            Result = TableAt->Scopes + Index;
            break;
        }
        TableAt = TableAt->Next;
    }
    return Result;
}

internal void
CollateFrame(debug_services* Services)
{
    scope_record_list* ListAt = Services->ScopeRecordsHead;
    while (ListAt != 0)
    {
        for (u32 i = 0; i < ListAt->Count; i++)
        {
            scope_record ScopeAt = ListAt->List[i];
            u64 ScopeAtDuration = ScopeAt.EndTime - ScopeAt.StartTime;
            
            collated_scope_record* CollatedScopeData = GetCollatedRecordForScope(Services, ScopeAt.IdentifierHash);
            Assert(CollatedScopeData != 0);
            CollatedScopeData->CallCount++;
            CollatedScopeData->TotalCycles += ScopeAtDuration;
            CollatedScopeData->LongestDuration = GSMax(ScopeAtDuration, CollatedScopeData->LongestDuration);
        }
        ListAt = ListAt->Next;
    }
    
    u32 SortListCount = Services->CollatedScopesCount;
    gs_radix_entry* SortList = PushArray(&Services->Transient, gs_radix_entry, SortListCount);
    
    u32 SortListAt = 0;
    collated_scopes_hash_table* TableAt = Services->CollatedScopes;
    while (TableAt != 0)
    {
        for (u32 i = 0; i < COLLATED_SCOPES_HASH_SIZE; i++)
        {
            if (TableAt->Hashes[i] == 0) { continue; }
            SortList[SortListAt].ID = TableAt->Hashes[i];
            SortList[SortListAt].Radix = TableAt->Scopes[i].TotalCycles;
            SortListAt += 1;
        }
        TableAt = TableAt->Next;
    }
    
    RadixSortInPlace(SortList, SortListCount);
    
    for (s32 i = SortListCount - 1; i >= 0; i--)
    {
        u64 Hash = SortList[i].ID;
        collated_scope_record* ScopeData = GetCollatedRecordForScope(Services, Hash);
        
        u64 AverageDuration = ScopeData->TotalCycles / ScopeData->CallCount;
        
        printf("Scope: %s\n", ScopeData->Identifier);
        printf("    %d / %d\n", SortListCount - i, SortListCount);
        printf("    Total Cycles:     %lld\n", ScopeData->TotalCycles);
        printf("    Call Count:       %lld\n", ScopeData->CallCount);
        printf("    Longest Duration: %lld\n", ScopeData->LongestDuration);
        printf("    Average Duration: %lld\n", AverageDuration);
    }
}

#define GS_PROFILER_H
#endif // GS_PROFILER_H