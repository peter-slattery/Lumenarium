// TODO
// [] - animation blending
// [] - delete a layer
// [] - will need a way to create an empty layer
// [] - get a list of all animation procs

#define ANIMATION_PROC(name) void name(assembly* Assembly, r32 Time)
typedef ANIMATION_PROC(animation_proc);

struct animation_block
{
    // TODO(Peter): Should we change this to frames??
    r32 StartTime;
    r32 EndTime;
    animation_proc* Proc;
    
    u32 Layer;
};

struct animation_block_handle
{
    s32 Index;
    // NOTE(Peter): Zero is invalid
    u32 Generation;
};

struct animation_block_entry
{
    u32 Generation;
    animation_block Block;
    free_list Free;
};

#define ANIMATION_SYSTEM_LAYERS_MAX 128
#define ANIMATION_SYSTEM_BLOCKS_MAX 128
struct animation_system
{
    animation_block_entry Blocks[ANIMATION_SYSTEM_BLOCKS_MAX];
    free_list FreeList;
    u32 BlocksCount;
    
    r32 Time;
    s32 LastUpdatedFrame;
    r32 SecondsPerFrame;
    
    b32 TimelineShouldAdvance;
    
    // :Temporary
    r32 AnimationStart;
    r32 AnimationEnd;
};

internal b32 
AnimationBlockHandlesAreEqual(animation_block_handle A, animation_block_handle B)
{
    b32 Result = ((A.Index == B.Index) && (A.Generation == B.Generation));
    return Result;
}

internal b32
AnimationBlockHandleIsValid(animation_block_handle Handle)
{
    b32 Result = Handle.Generation != 0;
    return Result;
}

internal void
InitializeAnimationSystem(animation_system* System)
{
    *System = {0};
    System->FreeList.Next = &System->FreeList;
}

inline b32
AnimationBlockIsFree(animation_block_entry Entry)
{
    // NOTE(Peter): If we've set Free.Next to zero, we've removed it from the
    // free list.
    b32 Result = Entry.Free.Next != 0;
    return Result;
}

internal animation_block_entry*
GetEntryAtIndex(u32 Index, animation_system* System)
{
    Assert(Index < System->BlocksCount);
    animation_block_entry* Result = System->Blocks + Index;
    return Result;
}

internal animation_block*
GetAnimationBlockWithHandle(animation_block_handle Handle, animation_system* System)
{
    animation_block* Result = 0;
    animation_block_entry* Entry = GetEntryAtIndex(Handle.Index, System);
    if (Entry && Entry->Generation == Handle.Generation)
    {
        Result = &Entry->Block;
    }
    return Result;
}

internal animation_block_handle
AddAnimationBlock(animation_block Block, animation_system* System)
{
    animation_block_handle Result = {0};
    
    if (System->FreeList.Next != 0 
        && System->FreeList.Next != &System->FreeList)
    {
        free_list* FreeEntry = System->FreeList.Next;
        Result.Index = FreeEntry->Index;
        System->FreeList.Next = FreeEntry->Next;
    }
    else
    {
        Assert(System->BlocksCount < ANIMATION_SYSTEM_BLOCKS_MAX);
        Result.Index = System->BlocksCount++;
    }
    
    animation_block_entry* Entry = GetEntryAtIndex(Result.Index, System);
    Entry->Generation += 1;
    Result.Generation = Entry->Generation;
    System->Blocks[Result.Index].Block = Block;
    System->Blocks[Result.Index].Free.Next = 0;
    
    return Result;
}

internal void
RemoveAnimationBlock(animation_block_handle Handle, animation_system* System)
{
    animation_block_entry* Entry = GetEntryAtIndex(Handle.Index, System);
    
    // NOTE(Peter): I'm pretty sure this doesn't need to be an assert but at the moment, there
    // is no reason why we shouldn't always be able to remove an entry when we request it. 
    // For now, I'm putting this assert here so we deal with this intentionally when the first
    // case comes up.
    // TODO: When we do deal with the above note, I'm guessing we want to return true or false
    // to signal if we were able to remove the entry or not so that the calling site can deal
    // with the removed reference
    Assert(Handle.Generation == Entry->Generation);
    
    Entry->Free.Index = Handle.Index;
    Entry->Free.Next = System->FreeList.Next;
    System->FreeList.Next = &Entry->Free;
}
