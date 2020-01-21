// File: gs_memory_arena.h
// Description: Single header file library that defines a push-only memory arena
// Author: Peter Slattery
// Date Created: 2019-12-22
// 
// 
// -----------------
//     Set Up
// -----------------
//
// Include 'gs_memory_arena.h' in a file and start using it! (Simple! Nice!)
//
// -----------------
//   Usage
// -----------------
// Simply create a memory_arena and use PushSize, PushStruct, or PushArray
// to allocate out of it.
// See Example Program below.
// 
// While there are options you can set (see Options below), the library adheres
// to a 'zero-is-initialization' policy, that is, a memory_arena initialized to
// zero, under all default options, will 'just work'. 
//
// Alignment:
// By default, the Push functions use 4 byte alignment
// If you need to control the alignment of an allocation, there are variants of the 
// Push functions that allow for this: PushSizeAligned, PushStructAligned, and PushArrayAligned
// These functions simply take a final parameter which specifies the alignment. 
// Note: Alignment must be a power of two
//
// -----------------
//   Options
// -----------------
// 
// DEBUG: 
// Define DEBUG for debug functionality.
// 
// To override the default assert function define GSMem_Assert(expression) 
// before inluding this file.
//
// GS_MEMORY_NO_STD_LIBS:
// if you don't want stdlib.h to be included, define GS_MEMORY_NO_STD_LIBS
// before including this file.
//   Note that if you do this, zero-is-initialization will no longer work for
//   memory_arenas. You must either:
//     1. Set each memory_arena's Alloc and Realloc so they can grow fields
//     2. Set each memory_arena's ExpansionRule to ExpansionRule_Disallowed
//   If DEBUG is defined, the program will assert if one of the 2 rules above
//   aren't followed.
//
// memory_arena.Alloc and memory_arena.Realloc
// By default, memory_arena's will use malloc and realloc to grow.
// You can override this by setting the Alloc and Realloc function pointers
// of a memory_arena. See the example program below for an implementation of this.
//
// GS_MEMORY_BUFFER_SIZE:
// This defines the minimum buffer size for memory_arena's. If an arena doesn't have
// room to fit an allocation, it will allocate a new buffer no smaller than GS_MEMORY_BUFFER_SIZE
// and place the allocation in the new buffer.
// By default this is 4096 bytes. To override, define GS_MEMORY_BUFFER_SIZE before including
// this file
//
// GS_MEMORY_TRACK_ALLOCATIONS:
// If you want to keep records of each allocation performed in every arena, define 
// GS_MEMORY_TRACK_ALLOCATIONS before including this file. 
// When defined, memory arenas gain fields that allow them to keep a list of every
// allocation they contain. It also adds a footer on the end of each allocation that
// can be checked to ensure there are no writes to allocations that overflow their bounds
// Note that calling ClearArena also clears this list
// You can then call AssertAllocationsNoOverflow occasionally throughout your program
// to check that no allocations have been written beyond their boundaries
//
//
// Example Program
// (this compiles - copy it into its own file though)
#if 0
// #include "gs_memory_arena.h"

// Places the characters 'gs' at the end of each allocation. This would allow for an external
// function to check that we haven't written past the end of an allocation
void* MallocWrapper(gs_mem_u32 Size) 
{ 
    int SizeWithFooter = Size + (sizeof(char) * 2);
    void* Result = malloc(SizeWithFooter);
    char* Footer = (char*)(Result + Size);
    Footer[0] = 'g';
    Footer[1] = 's';
    return Result;
}

void* ReallocWrapper(void* Address, gs_mem_u32 Size) 
{ 
    return realloc(Address, Size); 
}

int 
main(int ArgCount, char** Args)
{
    memory_arena Arena = {};
    // Uncomment these lines for an example of how you can implement custom allocation functions
    // Arena.Alloc = MallocWrapper; 
    // Arena.Realloc = ReallocWrapper;
    
    int ArrayLength = 10;
    
    int* A = PushArray(&Arena, int, ArrayLength);
    int* B = PushArray(&Arena, int, ArrayLength);
    int* C = PushArray(&Arena, int, ArrayLength);
    int* D = PushArrayAligned(&Arena, int, ArrayLength, 8);
    int* E = PushArrayAligned(&Arena, int, ArrayLength, 16);
    
    // Just ensure that we can actually write to each address of each array
    for (s32 i = 0; i < ArrayLength; i++)
    {
        A[i] = i;
        B[i] = i;
        C[i] = i;
        D[i] = i;
        E[i] = i;
    }
    
    ClearArena(&Arena);
    
    A = PushArray(&Arena, int, ArrayLength);
    for (s32 i = 0; i < ArrayLength; i++)
    {
        A[i] = i;
    }
    
    return 0;
}
#endif

// -------------------
//  Begin Library
// -------------------
#ifndef GS_MEMORY_ARENA_H

#ifndef GS_MEMORY_NO_STD_LIBS

// NOTE(Peter): We use this so that we can fall back on malloc and realloc
// in the event that a memory_arena needs to grow but doesn't have a 
// alloc or realloc function pointer assigned to it.
// 
// See GrowArena to see where this is used
//
#include <stdlib.h>

#endif

typedef unsigned char gs_mem_u8;
typedef unsigned int  gs_mem_u32;

typedef unsigned long long int gs_mem_u64;

#ifdef DEBUG
#if !defined(GSMem_Assert)
#define GSMem_Assert(expression) \
if(!(expression)) { \
*((int *)0) = 5; \
}

#endif
#else
#define GSMem_Assert(expression)
#endif

enum gs_memory_expansion_rule
{
    MemoryExpansion_Allowed, // Zero is initialization lets the memory grow on its own
    MemoryExpansion_OnlyIfFunctionsProvided,
    MemoryExpansion_Disallowed,
    MemoryExpansion_Count,
};

// NOTE(Peter):
// This rule is only here to allow for taking arena snapshots. The problem this solves
// is if you take a snapshot while there are 'holes' in memory_buffers behind the 
// most recently added memory_buffer, take a snapshot of that arena, then push something
// on that fits in one of those holes, we will fill the hole and be unable to track/free
// that addition via the snapshot construct. 
//
// By requiring that allocations in a buffer only come from the most recent memory_buffer
// we can very easily rewind the buffer to the correct location. 
// Hence FindAddress_InLastBufferOnly
enum gs_memory_find_address_rule
{
    FindAddress_InAnyBuffer,
    FindAddress_InLastBufferOnly,
    FindAddress_Count,
};

typedef void* gs_memory_alloc(gs_mem_u32 Size);
typedef void* gs_memory_realloc(void* Address, gs_mem_u32 OldSize, gs_mem_u32 NewSize);
typedef void gs_memory_free(void* Address, gs_mem_u32 Size);

#ifndef GS_MEMORY_BUFFER_SIZE
#define GS_MEMORY_BUFFER_SIZE 1024
#endif

#define GS_MEMORY_FOOTER_SIZE 4
#define GS_MEMORY_FOOTER_0 'g'
#define GS_MEMORY_FOOTER_1 's'
#define GS_MEMORY_FOOTER_2 'p'
#define GS_MEMORY_FOOTER_3 's'

struct tracked_allocation
{
    gs_mem_u8* Head;
    gs_mem_u8* Footer;
    char* File;
    gs_mem_u32 LineNumber;
};

#define GS_MEM_TRACKED_ALLOCATION_BUFFER_SIZE 512
struct tracked_allocation_buffer
{
    tracked_allocation Buffer[GS_MEM_TRACKED_ALLOCATION_BUFFER_SIZE];
};

struct memory_buffer
{
    gs_mem_u8* Buffer;
    gs_mem_u32 Size;
    gs_mem_u32 Used;
};

struct memory_arena
{
    memory_buffer* Buffers;
    gs_mem_u32 BuffersCount;
    
    gs_mem_u32 TotalUsed;
    gs_mem_u32 TotalSize;
    
    gs_memory_find_address_rule FindAddressRule;
    gs_memory_expansion_rule ExpansionRule;
    gs_memory_alloc* Alloc;
    gs_memory_realloc* Realloc;
    
#ifdef GS_MEMORY_TRACK_ALLOCATIONS
    tracked_allocation_buffer** AllocationBuffers;
    gs_mem_u32 AllocationBuffersCount;
    gs_mem_u32 AllocationsUsed;
#endif
};

struct address_and_buffer
{
    memory_buffer* Buffer;
    gs_mem_u64 Address;
    gs_mem_u32 SizeWithAlignment;
};

struct arena_snapshot
{
    gs_mem_u32 ArenaUsedAtSnapshot;
    gs_mem_u32 HeadBufferUsedAtSnapshot;
    gs_mem_u32 HeadBufferAtSnapshot;
    memory_arena* Arena;
    
#ifdef GS_MEMORY_TRACK_ALLOCATIONS
    gs_mem_u32 AllocationsUsedAtSnapshot;
#endif
};

static void
FreeMemoryArena(memory_arena* Arena, gs_memory_free* Free = 0)
{
    if (Free)
    {
        for (gs_mem_u32 i = 0; i < Arena->BuffersCount; i++)
        {
            memory_buffer* Buffer = Arena->Buffers + i;
            Free(Buffer->Buffer, Buffer->Size);
        }
        Free(Arena->Buffers, sizeof(memory_buffer) * Arena->BuffersCount);
    }
    else
    {
#ifdef GS_MEMORY_NO_STD_LIBS
        GSMem_Assert(0);
#else
        for (gs_mem_u32 i = 0; i < Arena->BuffersCount; i++)
        {
            memory_buffer* Buffer = Arena->Buffers + i;
            free(Buffer->Buffer);
        }
        free(Arena->Buffers);
#endif
    }
}

#define IsPowerOfTwo(v) ((v != 0) && ((v & (v - 1)) == 0))

inline gs_mem_u32
GetAlignmentOffset (gs_mem_u64 Address, gs_mem_u32 Alignment, gs_mem_u32 AlignmentMask)
{
    gs_mem_u32 AlignmentOffset = 0;
    if (Address & AlignmentMask)
    {
        AlignmentOffset = Alignment - (Address & AlignmentMask);
    }
    return AlignmentOffset;
}

static address_and_buffer
GetAlignedAddressInBuffer(memory_buffer* Buffer, gs_mem_u32 Size, gs_mem_u32 Alignment, gs_mem_u32 AlignmentMask)
{
    address_and_buffer Result = {};
    
    gs_mem_u64 HeadAddress = (gs_mem_u64)Buffer->Buffer + Buffer->Used;
    gs_mem_u32 AlignmentOffset = GetAlignmentOffset(HeadAddress, Alignment, AlignmentMask);
    gs_mem_u64 AlignedAddress = HeadAddress + AlignmentOffset;
    
    if (Buffer->Used + AlignmentOffset + Size <= Buffer->Size)
    {
        Result.Buffer = Buffer;
        Result.Address = AlignedAddress;
        Result.SizeWithAlignment = Size + AlignmentOffset;
    }
    
    return Result;
}

static address_and_buffer
FindAlignedAddressInBufferWithRoom(memory_arena* Arena, gs_mem_u32 Size, gs_mem_u32 Alignment, gs_mem_u32 AlignmentMask)
{
    address_and_buffer Result = {};
    for (gs_mem_u32 i = 0; i < Arena->BuffersCount; i++)
    {
        memory_buffer* At = Arena->Buffers + i;
        GSMem_Assert(At);
        
        address_and_buffer AddressInCurrentBuffer = GetAlignedAddressInBuffer(At, Size, Alignment, AlignmentMask);
        if (AddressInCurrentBuffer.Address != 0)
        {
            Result = AddressInCurrentBuffer;
            break;
        }
    } 
    return Result;
}

static gs_mem_u8*
ArenaAlloc(memory_arena* Arena, gs_mem_u32 Size)
{
    gs_mem_u8* Result = 0;
    
    if (Arena->Alloc)
    {
        Result = (gs_mem_u8*)Arena->Alloc(sizeof(gs_mem_u8) * Size);
    }
    else
    {
#ifdef GS_MEMORY_NO_STD_LIBS
        // NOTE(Peter): If you specify no std libs AND don't supply a allocation function
        // we should assert as this is an invalid codepath
        GSMem_Assert(0);
#else
        Result = (gs_mem_u8*)malloc(sizeof(gs_mem_u8) * Size);
#endif
    }
    
    return Result;
}

static gs_mem_u8*
ArenaRealloc(memory_arena* Arena, gs_mem_u8* Head, gs_mem_u32 OldSize, gs_mem_u32 NewSize)
{
    gs_mem_u8* Result = 0;
    
    if (Arena->Realloc != 0)
    {
        Result = (gs_mem_u8*)Arena->Realloc(Head, OldSize, NewSize);
    }
    else
    {
#ifdef GS_MEMORY_NO_STD_LIBS
        // NOTE(Peter): If you specify no std libs AND don't supply a reallocation function
        // we should assert as this is an invalid codepath
        GSMem_Assert(0);
#else
        Result = (gs_mem_u8*)realloc(Head, NewSize);
#endif
    }
    
    return Result;
}

static memory_buffer*
GrowArena(memory_arena* Arena, gs_mem_u32 SizeNeeded)
{
    GSMem_Assert(Arena->ExpansionRule != MemoryExpansion_Disallowed);
    if (Arena->ExpansionRule == MemoryExpansion_OnlyIfFunctionsProvided)
    {
        GSMem_Assert((Arena->Alloc != 0) && (Arena->Realloc != 0));
    }
    
    gs_mem_u32 NewBuffersCount = (Arena->BuffersCount + 1);
    gs_mem_u32 OldBuffersSize = sizeof(memory_buffer) * Arena->BuffersCount;
    gs_mem_u32 NewBuffersSize = sizeof(memory_buffer) * NewBuffersCount;
    Arena->Buffers = (memory_buffer*)ArenaRealloc(Arena, (gs_mem_u8*)Arena->Buffers, OldBuffersSize, NewBuffersSize);
    Arena->BuffersCount = NewBuffersCount;
    
    memory_buffer* NewBuffer = Arena->Buffers + (Arena->BuffersCount - 1);
    NewBuffer->Size = GS_MEMORY_BUFFER_SIZE;
    if (SizeNeeded > NewBuffer->Size)
    {
        NewBuffer->Size = SizeNeeded;
    }
    
    NewBuffer->Buffer = ArenaAlloc(Arena, sizeof(gs_mem_u8) * NewBuffer->Size);
    NewBuffer->Used = 0;
    
    Arena->TotalSize += NewBuffer->Size;
    return NewBuffer;
}

#ifdef GS_MEMORY_TRACK_ALLOCATIONS

#define DetermineAllocationSize(size) (size) + GS_MEMORY_FOOTER_SIZE
#define ClearAllocationsUsed(arena) (arena)->AllocationsUsed = 0
#define ClearAllocationsUsedToSnapshot(arena, snapshot) \
(arena)->AllocationsUsed = (snapshot).AllocationsUsedAtSnapshot;

static void
TrackAllocation(memory_arena* Arena, gs_mem_u8* Head, gs_mem_u32 Size, char* Filename, gs_mem_u32 LineNumber)
{
    gs_mem_u32 AllocationsMax = Arena->AllocationBuffersCount * GS_MEM_TRACKED_ALLOCATION_BUFFER_SIZE;
    if (Arena->AllocationsUsed >= AllocationsMax)
    {
        gs_mem_u32 NewAllocationBuffersCount = Arena->AllocationBuffersCount + 1;
        Arena->AllocationBuffers = (tracked_allocation_buffer**)ArenaRealloc(Arena, 
                                                                             (gs_mem_u8*)Arena->AllocationBuffers, 
                                                                             Arena->AllocationBuffersCount * sizeof(void*),
                                                                             NewAllocationBuffersCount * sizeof(void*));
        Arena->AllocationBuffersCount = NewAllocationBuffersCount;
        
        gs_mem_u32 NewBufferIndex = Arena->AllocationBuffersCount - 1;
        Arena->AllocationBuffers[NewBufferIndex] = (tracked_allocation_buffer*)ArenaAlloc(Arena, sizeof(tracked_allocation_buffer));
    }
    
    gs_mem_u32 AllocationIndex = Arena->AllocationsUsed++;
    gs_mem_u32 BufferIndex = AllocationIndex / GS_MEM_TRACKED_ALLOCATION_BUFFER_SIZE;
    gs_mem_u32 IndexInBuffer = AllocationIndex % GS_MEM_TRACKED_ALLOCATION_BUFFER_SIZE;
    tracked_allocation_buffer* Buffer = Arena->AllocationBuffers[BufferIndex];
    tracked_allocation* NewAllocationTracker = Buffer->Buffer + IndexInBuffer;
    
    NewAllocationTracker->Head = Head;
    
    NewAllocationTracker->Footer = Head + Size - GS_MEMORY_FOOTER_SIZE;
    NewAllocationTracker->Footer[0] = GS_MEMORY_FOOTER_0;
    NewAllocationTracker->Footer[1] = GS_MEMORY_FOOTER_1;
    NewAllocationTracker->Footer[2] = GS_MEMORY_FOOTER_2;
    NewAllocationTracker->Footer[3] = GS_MEMORY_FOOTER_3;
    
    NewAllocationTracker->File = Filename;
    NewAllocationTracker->LineNumber = LineNumber;
}

inline bool
VerifyAllocationNoOverflow (tracked_allocation Allocation)
{
    bool Result = ((Allocation.Footer[0] == GS_MEMORY_FOOTER_0) &&
                   (Allocation.Footer[1] == GS_MEMORY_FOOTER_1) &&
                   (Allocation.Footer[2] == GS_MEMORY_FOOTER_2) &&
                   (Allocation.Footer[3] == GS_MEMORY_FOOTER_3));
    return Result;
}

static void
AssertAllocationsNoOverflow (memory_arena Arena)
{
    for (gs_mem_u32 AllocationIndex = 0; 
         AllocationIndex< Arena.AllocationsUsed; 
         AllocationIndex++)
    {
        gs_mem_u32 BufferIndex = AllocationIndex / GS_MEM_TRACKED_ALLOCATION_BUFFER_SIZE;
        gs_mem_u32 IndexInBuffer = AllocationIndex % GS_MEM_TRACKED_ALLOCATION_BUFFER_SIZE;
        
        tracked_allocation_buffer* Buffer = Arena.AllocationBuffers[BufferIndex];
        tracked_allocation Allocation = Buffer->Buffer[IndexInBuffer];
        
        GSMem_Assert(VerifyAllocationNoOverflow(Allocation));
    }
}

#define PushSize(arena, size) PushSize_((arena), (size), 4, __FILE__, __LINE__)
#define PushArray(arena, type, length) (type*)PushSize_((arena), sizeof(type) * length, 4, __FILE__, __LINE__)
#define PushStruct(arena, type) (type*)PushSize_((arena), sizeof(type), 4, __FILE__, __LINE__)
#define PushSizeAligned(arena, size, alignment) PushSize_((arena), (size), (alignment), __FILE__, __LINE__)
#define PushArrayAligned(arena, type, length, alignment) (type*)PushSize_((arena), sizeof(type) * length, (alignment), __FILE__, __LINE__)
#define PushStructAligned(arena, type, alignment) (type*)PushSize_((arena), sizeof(type), (alignment), __FILE__, __LINE__)

#else // GS_MEMORY_TRACK_ALLOCATIONS

#define AssertAllocationsNoOverflow(arena)
#define DetermineAllocationSize(size) size
#define ClearAllocationsUsed(arena)
#define ClearAllocationsUsedToSnapshot(arena, snapshot)

#define TrackAllocation(arena, head, size, filename, linenumber) 

#define PushSize(arena, size) PushSize_((arena), (size))
#define PushArray(arena, type, length) (type*)PushSize_((arena), sizeof(type) * length)
#define PushStruct(arena, type) (type*)PushSize_((arena), sizeof(type))
#define PushSizeAligned(arena, size, alignment) PushSize_((arena), (size), (alignment))
#define PushArrayAligned(arena, type, length, alignment) (type*)PushSize_((arena), sizeof(type) * length, (alignment))
#define PushStructAligned(arena, type, alignment) (type*)PushSize_((arena), sizeof(type), (alignment))

#endif // GS_MEMORY_TRACK_ALLOCATIONS

static gs_mem_u8*
PushSize_(memory_arena* Arena, gs_mem_u32 Size, gs_mem_u32 Alignment = 4, char* Filename = 0, gs_mem_u32 LineNumber = 0)
{
    // ie. Alignment = 4 = 100 (binary)
    // 4 - 1 = 3 
    // 100 - 1 = 011 which is a mask of the bits we don't want set in the start address
    GSMem_Assert(IsPowerOfTwo(Alignment));
    gs_mem_u32 AlignmentMask = Alignment - 1;
    
    gs_mem_u32 AllocationSize = DetermineAllocationSize(Size);
    
    address_and_buffer ResultAddress = {};
    if (Arena->FindAddressRule == FindAddress_InAnyBuffer)
    {
        ResultAddress = FindAlignedAddressInBufferWithRoom(Arena, AllocationSize, Alignment, AlignmentMask);
    }
    else if (Arena->FindAddressRule == FindAddress_InLastBufferOnly
             && Arena->BuffersCount > 0)
    {
        memory_buffer* LastBuffer = Arena->Buffers + Arena->BuffersCount - 1;
        ResultAddress = GetAlignedAddressInBuffer(LastBuffer, Size, Alignment, AlignmentMask);
    }
    
    if (ResultAddress.Address == 0)
    {
        memory_buffer* Buffer = GrowArena(Arena, AllocationSize);
        ResultAddress = GetAlignedAddressInBuffer(Buffer, AllocationSize, Alignment, AlignmentMask);
    }
    GSMem_Assert(ResultAddress.Address != 0);
    GSMem_Assert((ResultAddress.Address & AlignmentMask) == 0);
    
    gs_mem_u8* Result = (gs_mem_u8*)ResultAddress.Address;
    ResultAddress.Buffer->Used += ResultAddress.SizeWithAlignment;
    Arena->TotalUsed += ResultAddress.SizeWithAlignment;
    
    TrackAllocation(Arena, Result, AllocationSize, Filename, LineNumber);
    
    return Result;
}

static void
ClearArena(memory_arena* Arena)
{
    for (gs_mem_u32 i = 0; i < Arena->BuffersCount; i++)
    {
        memory_buffer* At = Arena->Buffers + i;
        At->Used = 0;
    } 
    
    Arena->TotalUsed = 0;
    ClearAllocationsUsed(Arena);
}

static arena_snapshot
TakeSnapshotOfArena(memory_arena* Arena)
{
    Assert(Arena->FindAddressRule == FindAddress_InLastBufferOnly);
    
    arena_snapshot Result = {};
    Result.Arena = Arena;
    Result.ArenaUsedAtSnapshot = Arena->TotalUsed;
    if (Arena->BuffersCount > 0)
    {
        Result.HeadBufferAtSnapshot = Arena->BuffersCount - 1;
    }
    else
    {
        Result.HeadBufferAtSnapshot = 0;
    }
    
    memory_buffer* HeadBuffer = Arena->Buffers + Result.HeadBufferAtSnapshot;
    if (HeadBuffer)
    {
        Result.HeadBufferUsedAtSnapshot = HeadBuffer->Used;
    }
    
    return Result;
}

static void
ClearArenaToSnapshot(memory_arena* Arena, arena_snapshot Snapshot)
{
    Assert(Arena == Snapshot.Arena);
    
    memory_buffer* HeadBufferAtSnapshot = Arena->Buffers + Snapshot.HeadBufferAtSnapshot;
    if (HeadBufferAtSnapshot)
    {
        HeadBufferAtSnapshot->Used = Snapshot.HeadBufferUsedAtSnapshot;
        
        for (gs_mem_u32 i = Snapshot.HeadBufferAtSnapshot + 1; i < Arena->BuffersCount; i++)
        {
            memory_buffer* Buffer = Arena->Buffers + i;
            Buffer->Used = 0;
        }
    }
    
    Arena->TotalUsed = Snapshot.ArenaUsedAtSnapshot;
    ClearAllocationsUsedToSnapshot(Arena, Snapshot);
}
#define GS_MEMORY_ARENA_H
#endif // GS_MEMORY_ARENA_H