#ifndef GS_MEMORY_H

#define ArenaZeroStruct(data_ptr) ArenaZeroStruct_((u8*)data_ptr, sizeof(*data_ptr))
inline void
ArenaZeroStruct_ (u8* Base, s32 Size)
{
    u8* Iter = Base;
    for (s32 i = 0; i < Size; i++) { *Iter++ = 0; }
}

struct grow_arena_result
{
    u8* Base;
    s32 Size;
};

#define GROW_ARENA_MEMORY(name) grow_arena_result name(s32 Size)
typedef GROW_ARENA_MEMORY(grow_arena_memory);

#define FREE_ARENA_MEMORY(name) b32 name(u8* Base, s32 Size)
typedef FREE_ARENA_MEMORY(free_arena_memory);

struct memory_region_header
{
    memory_region_header* Prev;
    s32 Size;
    s32 Used;
    u8* Base;
};

inline b32
RegionCanFitSize (memory_region_header* Header, s32 Size)
{
    b32 Result = (Header->Used + Size) <= Header->Size;
    return Result;
}

inline b32
AddressIsInRegion (memory_region_header* Header, u8* Address)
{
    b32 Result = (Header->Base <= Address) && (Header->Base + Header->Used > Address);
    return Result;
}

#ifndef DEFAULT_MEMORY_ALIGNMENT
#define DEFAULT_MEMORY_ALIGNMENT (2 * sizeof(void*))
#endif

b32 GSMemIsPowerOfTwo (u64 Address)
{
    return (Address & (Address - 1)) == 0;
}

u64 AlignForward (u64 Base, u64 Align)
{
    u64 P, A, Modulo;
    
    Assert(GSMemIsPowerOfTwo(Align));
    
    P = Base;
    A = Align;
    Modulo = P & (A - 1);
    
    if (Modulo != 0)
    {
        P = P + (A - Modulo);
    }
    
    return P;
}

//////////////////////////////
//     Heap Memory
//////////////////////////////

// heap_memory_arena
// a growable memory arena that has two ways to interact with it: push and clear.
// Push: returns a free region of continguous memory. If the arenas GrowArenaProc function is set, this may
// get called in order to obtain enough free memory to fulfil the push request
// Clear: clears the entire memory arena. If the arena has been grown at any point, those subsequent
// regions of memory will be freed back to the system.
struct heap_memory_arena
{
    memory_region_header* CurrentRegion;
    
    s32 RegionMemorySize;
    grow_arena_memory* GrowArenaProc;
    free_arena_memory* FreeArenaMemoryProc;
};

static void
GrowHeapArena (heap_memory_arena* Arena, s32 RequestedSize)
{
    if (Arena->GrowArenaProc)
    {
        Assert(Arena->RegionMemorySize > 0);
        
        s32 GrowthSize = GSMax(RequestedSize, Arena->RegionMemorySize);
        grow_arena_result NewMemory = Arena->GrowArenaProc(GrowthSize + sizeof(memory_region_header));
        Assert(NewMemory.Size > 0);
        
        memory_region_header* Header = (memory_region_header*)NewMemory.Base;
        Header->Base = (u8*)NewMemory.Base + sizeof(memory_region_header);
        Header->Size = NewMemory.Size - sizeof(memory_region_header);
        Header->Used = 0;
        Header->Prev = Arena->CurrentRegion;
        Arena->CurrentRegion = Header;
    }
    else
    {
        InvalidCodePath;
    }
}

#define PushStruct(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PushArray(arena, type, count) (type*)PushSize_(arena, sizeof(type)*count)
static u8* 
PushSize_ (heap_memory_arena* Arena, s32 Size)
{
    if (!Arena->CurrentRegion) { GrowHeapArena(Arena, Size); }
    
    u8* CurrPointer = Arena->CurrentRegion->Base + Arena->CurrentRegion->Used; 
    u64 Offset = AlignForward((u64)CurrPointer, DEFAULT_MEMORY_ALIGNMENT);
    Offset -= (u64)(Arena->CurrentRegion->Base + Arena->CurrentRegion->Used);
    
    if (!RegionCanFitSize(Arena->CurrentRegion, Size + Offset))
    {
        // TODO(Peter): There might be empty space in the current region, its just not big enough for the
        // requested size. We should search backwards to see if there is enough space in a previous region
        // before growing the arena.
        
        GrowHeapArena(Arena, Size + Offset);
    }
    
    u8* Result = Arena->CurrentRegion->Base + Arena->CurrentRegion->Used + Offset;
    Arena->CurrentRegion->Used += Size + Offset;
    
    GSZeroMemory(Result, Size);
    
    return Result;
}

static void
InitHeapMemoryArena (heap_memory_arena* Arena, s32 RegionMemorySize, 
                     grow_arena_memory* GrowProc, free_arena_memory* FreeProc)
{
    ArenaZeroStruct(Arena);
    Arena->RegionMemorySize = RegionMemorySize;
    Arena->GrowArenaProc = GrowProc;
    Arena->FreeArenaMemoryProc = FreeProc;
}

static void
InitHeapMemoryArena (heap_memory_arena* Arena, u8* Base, s32 Size, 
                     s32 RegionMemorySize = 0, 
                     grow_arena_memory* GrowProc = 0, 
                     free_arena_memory* FreeProc = 0)
{
    Assert(Size > sizeof(memory_region_header));
    
    Arena->CurrentRegion = (memory_region_header*)Base;
    Arena->CurrentRegion->Base = Base + sizeof(memory_region_header);
    Arena->CurrentRegion->Size = Size - sizeof(memory_region_header);
    Arena->CurrentRegion->Used = 0;
    Arena->CurrentRegion->Prev = 0;
    
    Arena->RegionMemorySize = RegionMemorySize;
    Arena->GrowArenaProc = GrowProc;
    Arena->FreeArenaMemoryProc = FreeProc;
}

static void
ClearHeapMemoryArena (heap_memory_arena* Arena)
{
    if (!Arena->CurrentRegion) { return; }
    
    memory_region_header* CurrentHead = Arena->CurrentRegion;
    
    if (CurrentHead->Prev)
    {
        Assert(Arena->FreeArenaMemoryProc);
        while(CurrentHead->Prev)
        {
            memory_region_header* PrevHead = CurrentHead->Prev;
            Arena->FreeArenaMemoryProc((u8*)CurrentHead, CurrentHead->Size + sizeof(memory_region_header));
            CurrentHead = PrevHead;
        }
        
        Arena->CurrentRegion = CurrentHead;
    }
    
    Arena->CurrentRegion->Used = 0;
}


//////////////////////////////
//     Stack Memory
//////////////////////////////

struct stack_memory_region
{
    stack_memory_region* Prev;
};

// stack_memory_arena
// Push: returns a free region of continguous memory. If the arenas GrowArenaProc function is set, this may
// get called in order to obtain enough free memory to fulfil the push request
// Pop: frees the last region allocated on the stack, returning it to the region of memory available to 
// be used.
// Clear: clears the entire memory arena. If the arena has been grown at any point, those subsequent
// regions of memory will be freed back to the system.
struct stack_memory_arena
{
    memory_region_header* CurrentRegion;
    stack_memory_region* UsedList;
    
    s32 RegionMemorySize;
    grow_arena_memory* GrowArenaProc;
    free_arena_memory* FreeArenaMemoryProc;
};

static u8*
PushSize_ (stack_memory_arena* Arena, s32 Size)
{
    if (!Arena->CurrentRegion ||
        !RegionCanFitSize(Arena->CurrentRegion, Size))
    {
        if (Arena->GrowArenaProc)
        {
            Assert(Arena->RegionMemorySize > 0);
            
            grow_arena_result NewMemory = Arena->GrowArenaProc(Arena->RegionMemorySize + sizeof(memory_region_header));
            Assert(NewMemory.Size > 0);
            
            memory_region_header* Header = (memory_region_header*)NewMemory.Base;
            Header->Base = (u8*)NewMemory.Base + sizeof(memory_region_header);
            Header->Size = NewMemory.Size - sizeof(memory_region_header);
            Header->Used = 0;
            Header->Prev = Arena->CurrentRegion;
            Arena->CurrentRegion = Header;
        }
        else
        {
            InvalidCodePath;
        }
    }
    
    u8* Region = Arena->CurrentRegion->Base + Arena->CurrentRegion->Used;
    stack_memory_region* UsedListHeader = (stack_memory_region*)Region;
    UsedListHeader->Prev = Arena->UsedList;
    Arena->UsedList = UsedListHeader;
    
    u8* Result = Region + sizeof(stack_memory_region);
    Arena->CurrentRegion->Used += Size + sizeof(stack_memory_region);
    
    return Result;
}

// NOTE(Peter): Returns size available after the Pop operation
static s32
PopLast (stack_memory_arena* Arena)
{
    s32 Result = Arena->CurrentRegion->Size - Arena->CurrentRegion->Used;
    
    if (Arena->UsedList)
    {
        u8* LastHead = (u8*)Arena->UsedList;
        
        if (!AddressIsInRegion(Arena->CurrentRegion, LastHead) &&
            Arena->FreeArenaMemoryProc)
        {
            memory_region_header* PrevHeader = Arena->CurrentRegion->Prev;
            Arena->FreeArenaMemoryProc((u8*)Arena->CurrentRegion, 
                                       Arena->CurrentRegion->Size + sizeof(memory_region_header));
            Arena->CurrentRegion = PrevHeader;
            
        }
        
        Assert(LastHead >= Arena->CurrentRegion->Base && 
               LastHead <= Arena->CurrentRegion->Base + Arena->CurrentRegion->Size);
        
        stack_memory_region* PrevAlloc = Arena->UsedList->Prev;
        
        s32 SizeUsed = LastHead - Arena->CurrentRegion->Base;
        Arena->CurrentRegion->Used = SizeUsed;
        Result = Arena->CurrentRegion->Size - Arena->CurrentRegion->Used;
        
        Arena->UsedList = PrevAlloc;
    }
    
    return Result;
}

static void
InitStackMemoryArena (stack_memory_arena* Arena, s32 RegionMemorySize, 
                      grow_arena_memory* GrowProc, free_arena_memory* FreeProc)
{
    ArenaZeroStruct(Arena);
    Arena->RegionMemorySize = RegionMemorySize;
    Arena->GrowArenaProc = GrowProc;
    Arena->FreeArenaMemoryProc = FreeProc;
}

static void
InitStackMemoryArena (stack_memory_arena* Arena, u8* Base, s32 Size, 
                      s32 RegionMemorySize = 0, 
                      grow_arena_memory* GrowProc = 0, 
                      free_arena_memory* FreeProc = 0)
{
    Assert(Size > sizeof(memory_region_header));
    
    Arena->CurrentRegion = (memory_region_header*)Base;
    Arena->CurrentRegion->Base = Base + sizeof(memory_region_header);
    Arena->CurrentRegion->Size = Size - sizeof(memory_region_header);
    Arena->CurrentRegion->Used = 0;
    Arena->CurrentRegion->Prev = 0;
    
    Arena->RegionMemorySize = RegionMemorySize;
    Arena->GrowArenaProc = GrowProc;
    Arena->FreeArenaMemoryProc = FreeProc;
}

static void
ClearStackMemoryArena (stack_memory_arena* Arena)
{
    if (!Arena->CurrentRegion) { return; }
    
    memory_region_header* CurrentHead = Arena->CurrentRegion;
    
    if (CurrentHead->Prev)
    {
        Assert(Arena->FreeArenaMemoryProc);
        while(CurrentHead->Prev)
        {
            memory_region_header* PrevHead = CurrentHead->Prev;
            Arena->FreeArenaMemoryProc((u8*)CurrentHead, CurrentHead->Size + sizeof(memory_region_header));
            CurrentHead = PrevHead;
        }
        
        Arena->CurrentRegion = CurrentHead;
    }
    
    Arena->CurrentRegion->Used = 0;
    Arena->UsedList = 0;
}

//////////////////////////////
//     Pool Memory
//////////////////////////////

struct chunk_header
{
    chunk_header* Prev;
};

struct pool_memory_arena
{
    memory_region_header* CurrentRegion;
    s32 ChunkSize;
    chunk_header* FreeList;
    
    s32 RegionMemorySize;
    grow_arena_memory* GrowArenaProc;
    free_arena_memory* FreeArenaMemoryProc;
};

struct chunk_result
{
    s32 Size;
    u8* Base;
};

static chunk_result
PushChunk (pool_memory_arena* Arena)
{
    chunk_result Result = {};
    
    if (Arena->FreeList)
    {
        Result.Base = (u8*)Arena->FreeList;
        Result.Size = Arena->ChunkSize;
        
        Arena->FreeList = Arena->FreeList->Prev;
    }
    else
    {
        if (!RegionCanFitSize(Arena->CurrentRegion, Arena->ChunkSize))
        {
            if (Arena->GrowArenaProc)
            {
                grow_arena_result NewMemory = Arena->GrowArenaProc(Arena->RegionMemorySize + sizeof(memory_region_header));
                Assert(NewMemory.Size > 0);
                
                memory_region_header* Header = (memory_region_header*)NewMemory.Base;
                Header->Base = (u8*)NewMemory.Base + sizeof(memory_region_header);
                Header->Size = NewMemory.Size - sizeof(memory_region_header);
                Header->Used = 0;
                Header->Prev = Arena->CurrentRegion;
                Arena->CurrentRegion = Header;
            }
            else
            {
                InvalidCodePath;
            }
        }
        
        Result.Base = Arena->CurrentRegion->Base + Arena->CurrentRegion->Used;
        Result.Size = Arena->ChunkSize;
        
        Arena->CurrentRegion->Used += Arena->ChunkSize;
    }
    
    return Result;
}

static void
FreeChunk (pool_memory_arena* Arena, u8* Base, s32 Size)
{
    Assert(Arena->ChunkSize == Size);
    
    chunk_header* Header = (chunk_header*)Base;
    Header->Prev = Arena->FreeList;
    Arena->FreeList = Header;
}

static void
InitPoolMemoryArena (pool_memory_arena* Arena, s32 ChunkSize, s32 ChunksPerRegion, 
                     grow_arena_memory* GrowProc, free_arena_memory* FreeProc)
{
    Assert(ChunkSize > sizeof(chunk_header));
    
    ArenaZeroStruct(Arena);
    Arena->ChunkSize = ChunkSize;
    Arena->RegionMemorySize = ChunkSize * ChunksPerRegion;
    Arena->GrowArenaProc = GrowProc;
    Arena->FreeArenaMemoryProc = FreeProc;
}

static void
InitStackMemoryArena (pool_memory_arena* Arena, u8* Base, s32 Size, 
                      s32 ChunkSize, s32 ChunksPerRegion,
                      grow_arena_memory* GrowProc = 0, 
                      free_arena_memory* FreeProc = 0)
{
    Assert(Size > sizeof(memory_region_header));
    Assert(Size % ChunkSize == ChunksPerRegion);
    
    Arena->CurrentRegion = (memory_region_header*)Base;
    Arena->CurrentRegion->Base = Base + sizeof(memory_region_header);
    Arena->CurrentRegion->Size = Size - sizeof(memory_region_header);
    Arena->CurrentRegion->Used = 0;
    Arena->CurrentRegion->Prev = 0;
    
    Arena->ChunkSize = ChunkSize;
    Arena->RegionMemorySize = ChunkSize * ChunksPerRegion;
    Arena->GrowArenaProc = GrowProc;
    Arena->FreeArenaMemoryProc = FreeProc;
}

#define GS_MEMORY_H
#endif // GS_MEMORY_H
