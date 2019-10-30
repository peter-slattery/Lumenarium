#ifndef GS_MEMORY_H

#ifndef GS_LANGUAGE_H

typedef uint8_t u8;
typedef int8_t s8;
typedef uint32_t u32;
typedef int32_t s32;

internal void
GSMemSet (u8* Base, s32 Value, s32 Count)
{
    u8* Cursor = Base;
    for (s32 i = 0; i < Count; i++)
    {
        *Cursor++ = Value;
    }
}

internal void
GSMemCopy (u8* Source, u8* Destination, s32 Count)
{
    u8* Src = Source;
    u8* Dst = Destination;
    
    for (s32 i = 0; i < Count; i++)
    {
        *Dst++ = *Src++;
    }
}

#endif // GS_LANGUAGE_H

#ifndef GS_PLATFORM_H

#define PLATFORM_MEMORY_NO_ERROR 0
#define PLATFORM_MEMORY_ERROR 1
struct platform_memory_result
{
    s32 Size;
    u8* Base;
    s32 Error;
};

#define PLATFORM_ALLOC(name) platform_memory_result name(s32 Size)
typedef PLATFORM_ALLOC(platform_alloc);

// Returns 1 if successful, 0 otherwise
#define PLATFORM_FREE(name) b32 name(u8* Memory, s32 Size)
typedef PLATFORM_FREE(platform_free);

#endif // GS_PLATFORM_H

#if !defined Assert && defined DEBUG
#define Assert(expression) if(!(expression)){ *((int *)0) = 5; }
#define InvalidCodePath Assert(0)
#endif

#define MEMORY_REGION_PAGE_SIZE Megabytes(1)
struct memory_region
{
    u8* Base;
    u32 Size;
    u32 Used;
    memory_region* PreviousRegion;
};

struct memory_arena
{
    memory_region* CurrentRegion;
    platform_alloc* PlatformAlloc;
};

internal memory_region*
BootstrapRegionOntoMemory (u8* Memory, s32 Size)
{
    Assert(Size > sizeof(memory_region));
    memory_region* Result = (memory_region*)Memory;
    Result->Base = Memory + sizeof(memory_region);
    Result->Size = Size - sizeof(memory_region);
    Result->Used = 0;
    Result->PreviousRegion = 0;
    return Result;
}

#define PushStruct(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PushArray(arena, type, count) (type*)PushSize_(arena, sizeof(type)*count)
#define PushSize(arena, size) PushSize_(arena, size)
static u8* 
PushSize_ (memory_arena* Arena, u32 Size)
{
    memory_region* PushOntoRegion = Arena->CurrentRegion;
    
    if (!PushOntoRegion || PushOntoRegion->Used + Size > PushOntoRegion->Size)
    {
        // NOTE(Peter): we only search backwards if the item doesn't already fit in the most recent spot. This way, memory allocated
        // one after another is more likely to be contiguous. You can expect that two allocations performed back to back are also next
        // to eachother in memory most of the time. 
        if (PushOntoRegion)
        {
            // NOTE(Peter): Search backwards through previous regions to see if there is a region allocated that has enough room
            // to fit this allocation
            memory_region* PreviousRegion = Arena->CurrentRegion->PreviousRegion;
            while (PreviousRegion)
            {
                if (PreviousRegion->Used + Size <= PreviousRegion->Size)
                {
                    PushOntoRegion = PreviousRegion;
                    break;
                }
                PreviousRegion = PreviousRegion->PreviousRegion;
            }
        }
        
        if (!PushOntoRegion || PushOntoRegion->Used + Size > PushOntoRegion->Size)
        {
            if (Arena->PlatformAlloc != 0)
            {
                // NOTE(Peter): Probably want to have this be a multiple of some minimum size so that we aren't constantly
                // allocating new pages.
                s32 SizeNeeded = Size + sizeof(memory_region);
                s32 RegionPagesNeeded = IntegerDivideRoundUp(SizeNeeded, MEMORY_REGION_PAGE_SIZE); 
                s32 SizeToAllocate = RegionPagesNeeded * MEMORY_REGION_PAGE_SIZE;
                
                platform_memory_result AllocResult = Arena->PlatformAlloc(SizeToAllocate);
                Assert(AllocResult.Error == PLATFORM_MEMORY_NO_ERROR);
                Assert(AllocResult.Size >= SizeNeeded);
                
                memory_region* NewRegion = BootstrapRegionOntoMemory(AllocResult.Base, AllocResult.Size);
                NewRegion->PreviousRegion = Arena->CurrentRegion;
                Arena->CurrentRegion = NewRegion;
                PushOntoRegion = Arena->CurrentRegion;
            }
            else
            {
                // NOTE(Peter): We ran out of memory in a memory arena that cannot/should not grow
                InvalidCodePath;
            }
        }
    }
    
    u8* Result = PushOntoRegion->Base + PushOntoRegion->Used;
    PushOntoRegion->Used += Size;
    
    return Result;
}

static void
InitMemoryArena (memory_arena* Arena, u8* Base, u32 Size, platform_alloc* PlatformAlloc)
{
    if (Base)
    {
        Arena->CurrentRegion = BootstrapRegionOntoMemory(Base, Size);
    }
    Arena->PlatformAlloc = PlatformAlloc;
}

static memory_arena* 
BootstrapArenaIntoMemory (u8* Memory, u32 Size)
{
    Assert(Size > sizeof(memory_arena));
    // NOTE(Peter): takes in a block of memory, places a memory arena at the head, and gives
    // the arena access to the rest of the block to use.
    memory_arena* Result = (memory_arena*)Memory;
    *Result = {};
    InitMemoryArena(Result, Memory + sizeof(memory_arena), Size - sizeof(memory_arena), 0);
    return Result;
}

static memory_arena
AllocateNonGrowableArenaWithSpace(platform_alloc* PlatformAlloc, s32 SizeNeeded)
{
    // TODO(Peter): This causes a leak currently. If you don't free the whole region later, you'll end up with 
    // the memory_region still being in memory. Should probably just make the first memory region be a member
    // variable, not a pointer, in the memory_arena struct.
    
    memory_arena Result = {};
    
    s32 AllocateSize = SizeNeeded + sizeof(memory_region);
    platform_memory_result Memory = PlatformAlloc(AllocateSize);
    Assert(Memory.Error == PLATFORM_MEMORY_NO_ERROR);
    Assert(Memory.Size == AllocateSize);
    
    InitMemoryArena(&Result, Memory.Base, Memory.Size, 0);
    
    return Result;
}

static void
ClearMemoryRegion (memory_region* Region)
{
#if 0
    // NOTE(Peter): Turn this on occasionally. This is a big time sink but it forces us into 
    // correct memory usage since there's no error reporting for accessing memory the arena thinks
    // is unused. At least now, it'll be zero's.
    GSMemSet(Region->Base, 0, Region->Size);
#endif
    Region->Used = 0;
}

static void
ClearArena (memory_arena* Arena)
{
    memory_region* CurrentRegion = Arena->CurrentRegion;
    while (CurrentRegion)
    {
        ClearMemoryRegion(CurrentRegion);
        CurrentRegion = CurrentRegion->PreviousRegion;
    }
}

struct arena_snapshot
{
    memory_region* CurrentRegion;
    u32 UsedAtSnapshot;
};

static arena_snapshot
TakeSnapshotOfArena (memory_arena Arena)
{
    arena_snapshot Result = {};
    Result.CurrentRegion = Arena.CurrentRegion;
    Result.UsedAtSnapshot = Arena.CurrentRegion->Used;
    return Result;
};

static void
ZeroArenaToSnapshot (memory_arena* Arena, arena_snapshot Snapshot)
{
    memory_region* RegionCursor = Arena->CurrentRegion;
    while (RegionCursor && RegionCursor != Snapshot.CurrentRegion)
    {
        GSZeroMemory(RegionCursor->Base, RegionCursor->Size);
        RegionCursor = RegionCursor->PreviousRegion;
    }
    
    Assert(RegionCursor == Snapshot.CurrentRegion);
    GSZeroMemory(RegionCursor->Base + Snapshot.UsedAtSnapshot, 
                 RegionCursor->Used - Snapshot.UsedAtSnapshot);
}

static void
ClearArenaToSnapshot (memory_arena* Arena, arena_snapshot Snapshot)
{
    memory_region* RegionCursor = Arena->CurrentRegion;
    while (RegionCursor && RegionCursor != Snapshot.CurrentRegion)
    {
        RegionCursor->Used = 0;
        RegionCursor = RegionCursor->PreviousRegion;
    }
    
    Assert(RegionCursor == Snapshot.CurrentRegion);
    RegionCursor->Used = Snapshot.UsedAtSnapshot;
}

//
//  Basic Memory Arena
//  A no-bookkeeping overhead version of the memory_arena above.
//

struct static_memory_arena
{
    u8* Base;
    u32 Size;
    u32 Used;
};

static static_memory_arena
CreateMemoryArena (u8* Base, u32 Size)
{
    static_memory_arena Result = {};
    Result.Base = Base;
    Result.Size = Size;
    Result.Used = 0;
    return Result;
}

static u8* 
PushSize_ (static_memory_arena* Arena, u32 Size)
{
    Assert(Arena->Used + Size <= Arena->Size);
    u8* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return Result;
}

//
//  Tracked Array Implementation
//

#define ARRAY_CHECKSUM 0x51bada7b
struct array_header_
{
    u32 Size;
    s32 ElementMax;
    s32 ElementCount;
    s32 ElementSize;
    u32 Checksum;
};

#define gs_PushArray(arena, type, size) (type*)gs_PushArray_(arena, sizeof(type), size)

static u8*
gs_PushArray_ (memory_arena* Arena, u32 StepSize, u32 Count)
{
    u32 ArrayFootprint = sizeof(array_header_) + (StepSize * Count);
    array_header_* Header = (array_header_*)PushSize_(Arena, ArrayFootprint);
    
    array_header_* Body = Header + 1;
    u8* Result = (u8*)(Body);
    
    Header->Size = Count * StepSize;
    Header->ElementMax = Count;
    Header->ElementSize = StepSize;
    Header->ElementCount = 0;
    Header->Checksum = ARRAY_CHECKSUM;
    
    return Result;
}

#define gs_ArrayHeader_(array) (((array_header_*)array) - 1)

#ifdef DEBUG
#define gs_ArrayCheck(array) Assert(!array || gs_ArrayHeader_(array)->Checksum == ARRAY_CHECKSUM)
#else
#define gs_ArrayCheck(array) 
#endif

#define gs_ArrayCount(array) gs_ArrayCount_((u8*)array)
static s32
gs_ArrayCount_ (u8* Base)
{
    gs_ArrayCheck(Base);
    return gs_ArrayHeader_(Base)->ElementCount;
}

#define gs_ArrayMax(array) gs_ArrayMax_((u8*)array)
static s32
gs_ArrayMax_ (u8* Base)
{
    gs_ArrayCheck(Base);
    return gs_ArrayHeader_(Base)->ElementMax;
}

#define gs_ArrayAdd(array) ( gs_PushArrayElement_((u8*)array), (array) + (gs_ArrayCount(array) - 1) )
#define gs_ArrayPush(array, ele) *( gs_ArrayAdd(array) ) = (ele)

static void*
gs_PushArrayElement_ (u8* Base)
{
    gs_ArrayCheck(Base);
    Assert(gs_ArrayHeader_(Base)->ElementCount + 1 <= gs_ArrayHeader_(Base)->ElementMax);
    
    void* Result = (void*)(Base + (gs_ArrayHeader_(Base)->ElementCount * gs_ArrayHeader_(Base)->ElementSize));
    gs_ArrayHeader_(Base)->ElementCount++;
    
    return Result;
}



#define GS_MEMORY_H
#endif // GS_MEMORY_H