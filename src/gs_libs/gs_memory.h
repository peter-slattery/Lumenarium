/* date = May 10th 2021 10:19 pm */

#ifndef GS_MEMORY_H
#define GS_MEMORY_H

#if !defined(Assert)
# define Assert(c)
#endif

#if !defined(InvalidCodePath)
# define InvalidCodePath
#endif

#if !defined(GS_MEMORY_PROFILE_FUNC)
# define GS_MEMORY_PROFILE_FUNC
#endif

#define MemoryClearArray(b,t,s) MemoryClear((u8*)(b), sizeof(t) * (s))
internal void
MemoryClear(u8* Base, u64 Size)
{
  for (u64 i = 0; i < Size; i++) Base[i] = 0;
}

#ifndef DEBUG_LOC

typedef struct gs_debug_loc
{
  char* File;
  char* Function;
  u32 Line;
} gs_debug_loc;

# define DEBUG_LOC gs_debug_loc{ (char*)__FILE__, (char*)__FUNCTION__, __LINE__ }

#endif
//
// Debug Info
//

typedef struct gs_debug_memory_allocation
{
  gs_debug_loc Loc;
  u64 ArenaHash;
  u64 Size;
  
  struct gs_debug_memory_allocation* Next;
} gs_debug_memory_allocation;

typedef struct gs_debug_arena_info
{
  char* ArenaName;
  u64 AllocationsCount;
  u64 TotalSize;
} gs_debug_arena_info;

typedef struct gs_debug_allocations_list
{
  gs_debug_memory_allocation* Root;
  gs_debug_memory_allocation* Head;
  u64 AllocationsSizeTotal;
  u64 AllocationsCount;
  
  u64 ArenaHashesCount;
  u64* ArenaHashes;
  gs_debug_arena_info* ArenaInfos;
} gs_debug_allocations_list;

//
// Allocator
//

#define PLATFORM_ALLOC(name) void* name(u64 Size, u64* ResultSize, u8* UserData)
typedef PLATFORM_ALLOC(platform_alloc);

#define PLATFORM_FREE(name) void name(void* Base, u64 Size, u8* UserData)
typedef PLATFORM_FREE(platform_free);

typedef struct gs_allocator
{
  platform_alloc* PAlloc;
  platform_free*  PFree;
  u8* UserData;
  
  gs_debug_allocations_list* DEBUGAllocList;
} gs_allocator;

PLATFORM_ALLOC(AllocNoOp)
{
  GS_MEMORY_PROFILE_FUNC;
  if (ResultSize) *ResultSize = 0;
  return 0;
}

PLATFORM_FREE(FreeNoOp)
{
  GS_MEMORY_PROFILE_FUNC;
  return;
}

internal u64
GSMemoryHash(char* Str, u64 Len)
{
  u64 Hash = 5381;
  for (u32 i = 0; i < Len; i++)
  {
    Hash = ((Hash << 5) + Hash) + Str[i];
  }
  return Hash;
}

#define Alloc(a,s,an) Alloc_((a),(s),DEBUG_LOC, (char*)(an)).Memory
#define AllocStruct(a,t,an) (t*)Alloc_((a),sizeof(t),DEBUG_LOC, (char*)(an)).Memory
#define AllocArray(a,t,c,an) (t*)Alloc_((a),sizeof(t)*(c),DEBUG_LOC, (char*)(an)).Memory
#define AllocData(a,s,an) Alloc_((a),(s),DEBUG_LOC, (char*)(an))

internal gs_data
Alloc_(gs_allocator A, u64 Size, gs_debug_loc Loc, char* ArenaName)
{
  GS_MEMORY_PROFILE_FUNC;
  
  gs_data Result = {};
  Result.Memory = (u8*)A.PAlloc(Size, &Result.Size, A.UserData);
  
  // Debug
  {
    u64 ArenaHash = GSMemoryHash(ArenaName, CStringLength(ArenaName));
    
    gs_debug_memory_allocation* Allocation = (gs_debug_memory_allocation*)A.PAlloc(sizeof(gs_debug_memory_allocation*), 0, A.UserData);
    Allocation->Loc = Loc;
    Allocation->Size = Size;
    Allocation->ArenaHash = ArenaHash;
    SLLPushOrInit(A.DEBUGAllocList->Root, A.DEBUGAllocList->Head, Allocation);
    
    A.DEBUGAllocList->AllocationsSizeTotal += Size;
    A.DEBUGAllocList->AllocationsCount += 1;
    
    u64 ArenaStartIndex = ArenaHash % A.DEBUGAllocList->ArenaHashesCount; 
    u64 ArenaIndex = ArenaStartIndex;
    while (A.DEBUGAllocList->ArenaHashes[ArenaIndex] != 0 &&
           A.DEBUGAllocList->ArenaHashes[ArenaIndex] != ArenaHash)
    {
      ArenaIndex = (ArenaIndex + 1) % A.DEBUGAllocList->ArenaHashesCount;
      
      // NOTE(PS): this means you've created enough arena's to fill up every
      // slot in the arena hash table. Go increase the size of of DEBUG_ALLOCATOR_ARENA_MAX
      Assert(ArenaIndex != ArenaStartIndex);
    }
    
    if (A.DEBUGAllocList->ArenaHashes[ArenaIndex] == 0) 
    {
      A.DEBUGAllocList->ArenaHashes[ArenaIndex] = ArenaHash;
      
      gs_debug_arena_info AI = {};
      AI.ArenaName = ArenaName;
      A.DEBUGAllocList->ArenaInfos[ArenaIndex] = AI;
    }
    
    A.DEBUGAllocList->ArenaInfos[ArenaIndex].AllocationsCount += 1;
    A.DEBUGAllocList->ArenaInfos[ArenaIndex].TotalSize += Size;
  }
  
  
  
  return Result;
}

#define AllocString(a,s,l) AllocString_((a), (s), DEBUG_LOC, (l)) 
internal gs_string
AllocString_(gs_allocator A, u64 Size, gs_debug_loc Loc, char* LocString)
{
  gs_string Result = {};
  Result.Str = (char*)Alloc_(A, sizeof(char)*Size, Loc, LocString).Memory;
  Result.Length = 0;
  Result.Size = Size;
  return Result;
}

#define Free(a,b,s) Free_((a),(b),(s),DEBUG_LOC)
#define FreeStruct(a,b,t) Free_((a),(u8*)(b),sizeof(t),DEBUG_LOC)
#define FreeArray(a,b,t,c) Free_((a),(u8*)(b),sizeof(t)*(c),DEBUG_LOC)
internal void
Free_(gs_allocator A, u8* Base, u64 Size, gs_debug_loc Loc)
{
  GS_MEMORY_PROFILE_FUNC;
  
  A.PFree(Base, Size, A.UserData);
}

// NOTE(PS): cast function and struct pointers to proper data types
// for convenience
#define AllocatorCreate(a,f,u) AllocatorCreate_((platform_alloc*)(a),(platform_free*)(f),(u8*)(u))
internal gs_allocator
AllocatorCreate_(platform_alloc* PAlloc, platform_free* PFree, u8* UserData)
{
  gs_allocator Result = {};
  Result.PAlloc = PAlloc;
  Result.PFree = PFree;
  Result.UserData = UserData;
  
  if (!PAlloc) Result.PAlloc = AllocNoOp;
  if (!PFree)  Result.PFree  = FreeNoOp;
  
  // @DEBUG
#define DEBUG_ALLOCATOR_ARENA_MAX 256
  Result.DEBUGAllocList = (gs_debug_allocations_list*)PAlloc(sizeof(gs_debug_allocations_list*), 0, UserData);
  
  Result.DEBUGAllocList->ArenaHashesCount = DEBUG_ALLOCATOR_ARENA_MAX;
  Result.DEBUGAllocList->ArenaHashes = (u64*)PAlloc(sizeof(u64*) * DEBUG_ALLOCATOR_ARENA_MAX, 0, UserData);
  Result.DEBUGAllocList->ArenaInfos = (gs_debug_arena_info*)PAlloc(sizeof(gs_debug_arena_info) * DEBUG_ALLOCATOR_ARENA_MAX, 0, UserData);
  return Result;
}

//
// Memory Cursor
//

typedef struct gs_memory_cursor
{
  gs_data Data;
  u64 Position;
} gs_memory_cursor;

internal gs_memory_cursor
MemoryCursorCreate(u8* Base, u64 Size)
{
  GS_MEMORY_PROFILE_FUNC;
  
  gs_memory_cursor Result = {};
  Result.Data.Memory = Base;
  Result.Data.Size = Size;
  Result.Position = 0;
  
  return Result;
}
internal gs_memory_cursor
MemoryCursorCreateFromData(gs_data Data)
{
  GS_MEMORY_PROFILE_FUNC;
  return MemoryCursorCreate(Data.Memory, Data.Size);
}

internal u64
MemoryCursorRoomLeft(gs_memory_cursor Cursor)
{
  GS_MEMORY_PROFILE_FUNC;
  
  u64 Result = 0;
  if (Cursor.Data.Size >= Cursor.Position) 
  {
    Result = Cursor.Data.Size - Cursor.Position;
  }
  return Result;
}

internal bool
MemoryCursorHasRoom(gs_memory_cursor Cursor)
{
  GS_MEMORY_PROFILE_FUNC;
  
  u64 RoomLeft = MemoryCursorRoomLeft(Cursor);
  bool Result = RoomLeft > 0;
  return Result;
}

internal bool
MemoryCursorCanPush(gs_memory_cursor Cursor, u64 Size)
{
  GS_MEMORY_PROFILE_FUNC;
  
  u64 RoomLeft = MemoryCursorRoomLeft(Cursor);
  bool Result = RoomLeft >= Size;
  return Result;
}

#define MemoryCursorPushSize(c,s) MemoryCursorPushSize_((c),(s))
#define MemoryCursorPushStruct(c,s) (s*)MemoryCursorPushSize_((c),sizeof(s)).Memory
#define MemoryCursorPushArray(c,s,l) (s*)MemoryCursorPushSize_((c),sizeof(s)*(l)).Memory
internal gs_data
MemoryCursorPushSize_(gs_memory_cursor* C, u64 Size)
{
  GS_MEMORY_PROFILE_FUNC;
  
  gs_data Result = {0};
  if (MemoryCursorCanPush(*C, Size))
  {
    Result.Memory = C->Data.Memory + C->Position;
    Result.Size = Size,
    C->Position += Size;
  }
  return Result;
}

internal gs_data
MemoryCursorAlign(gs_memory_cursor* C, u64 Alignment)
{
  GS_MEMORY_PROFILE_FUNC;
  
  u64 Position = RoundUpTo64(C->Position, Alignment);
  if (Position > C->Data.Size)
  {
    Position = C->Data.Size;
  }
  u64 AlignmentDist = Position - C->Position;
  gs_data Result = MemoryCursorPushSize_(C, AlignmentDist);
  return Result;
}

#define MemoryCursorWriteValue(c,t,v) *PushStructOnCursor((c),(t)) = (v)
internal void
MemoryCursorWriteBuffer(gs_memory_cursor* C, gs_data Buffer)
{
  GS_MEMORY_PROFILE_FUNC;
  
  gs_data Dest = MemoryCursorPushSize(C, Buffer.Size);
  if (Dest.Size == Buffer.Size)
  {
    CopyMemoryTo(Buffer.Memory, Dest.Memory, Buffer.Size);
  }
}

internal void
MemoryCursorPopSize(gs_memory_cursor* C, u64 Size)
{
  GS_MEMORY_PROFILE_FUNC;
  
  u64 SizeToPop = Size;
  if (SizeToPop > C->Position) 
  {
    SizeToPop = C->Position;
  }
  
  C->Position -= SizeToPop;
}

internal void
MemoryCursorReset(gs_memory_cursor* C)
{
  GS_MEMORY_PROFILE_FUNC;
  
  C->Position = 0;
}

//
// Memory Arena
//

typedef struct gs_memory_cursor_sll
{
  gs_memory_cursor Cursor;
  struct gs_memory_cursor_sll* Next;
} gs_memory_cursor_sll;

typedef struct gs_memory_arena
{
  u64 ChunkSize;
  u64 Alignment;
  char* ArenaName;
  
  gs_memory_cursor_sll* CursorsRoot;
  gs_memory_cursor_sll* CursorsHead;
  
  struct gs_memory_arena* Parent;
  gs_allocator Allocator;
  u8* UserData;
  // TODO: some sort of GrowArena function
} gs_memory_arena;

internal gs_memory_arena
MemoryArenaCreate(u64 ChunkSize, u64 Alignment, gs_allocator Allocator, gs_memory_arena* Parent, u8* UserData, char* Name)
{
  GS_MEMORY_PROFILE_FUNC;
  
  gs_memory_arena Result = {};
  Result.ChunkSize = ChunkSize;
  Result.Alignment = Alignment;
  Result.Allocator = Allocator;
  Result.Parent = Parent;
  Result.UserData = UserData;
  Result.ArenaName = Name;
  
  return Result;
}

internal gs_data PushSize_(gs_memory_arena* Arena, u64 Size, gs_debug_loc Loc);

internal gs_memory_cursor*
MemoryArenaPushCursor(gs_memory_arena* Arena, u64 MinSize, gs_debug_loc Loc)
{
  GS_MEMORY_PROFILE_FUNC;
  
  gs_memory_cursor* Result = 0;
  
  u64 CursorSize = MinSize;
  if (CursorSize < Arena->ChunkSize)
  {
    CursorSize = Arena->ChunkSize;
  }
  u64 AllocSize = CursorSize + sizeof(gs_memory_cursor_sll);
  
  gs_data CursorMemory = {0};
  if (Arena->Parent)
  {
    CursorMemory = PushSize_(Arena->Parent, AllocSize, Loc);
  } else if (Arena->UserData) {
    // TODO(PS): implement custom MemoryArenaAllocCursor functionality
    InvalidCodePath;
  } else {
    Assert(Arena->Allocator.PAlloc);
    CursorMemory = Alloc_(Arena->Allocator, AllocSize, Loc, Arena->ArenaName);
  }
  
  gs_memory_cursor_sll* CursorEntry = (gs_memory_cursor_sll*)CursorMemory.Memory;
  if (!Arena->CursorsRoot)
  {
    Arena->CursorsRoot = CursorEntry;
  }
  if (Arena->CursorsHead)
  {
    Arena->CursorsHead->Next = CursorEntry;
  }
  Arena->CursorsHead = CursorEntry;
  
  u8* CursorBase = (u8*)(CursorEntry + 1);
  CursorEntry->Cursor = MemoryCursorCreate(CursorBase, CursorSize);
  Result = &CursorEntry->Cursor;
  
  return Result;
}

#define PushSize(a,s) PushSize_((a), (s), DEBUG_LOC)
#define PushStruct(a,t) (t*)PushSize_((a), sizeof(t), DEBUG_LOC).Memory
#define PushArray(a,t,c) (t*)PushSize_((a), sizeof(t) * (c), DEBUG_LOC).Memory
#define PushString(a,c) gs_string{ PushArray((a),char,(c)), 0, (c) }

internal gs_data
PushSize_(gs_memory_arena* Arena, u64 Size, gs_debug_loc Loc)
{
  GS_MEMORY_PROFILE_FUNC;
  
  gs_data Result = {0};
  if (Size > 0)
  {
    gs_memory_cursor* Cursor = 0;
    for (gs_memory_cursor_sll* C = Arena->CursorsRoot;
         C != 0;
         C = C->Next)
    {
      if (MemoryCursorCanPush(C->Cursor, Size))
      {
        Cursor = &C->Cursor;
        break;
      }
    }
    
    // NOTE(PS): We didn't find a cursor with enough room
    // for the allocation being requested
    if (!Cursor)
    {
      Cursor = MemoryArenaPushCursor(Arena, Size, Loc);
    }
    Assert(Cursor);
    Assert(MemoryCursorCanPush(*Cursor, Size));
    
    Result = MemoryCursorPushSize(Cursor, Size);
    
    gs_data Alignment = MemoryCursorAlign(Cursor, Arena->Alignment);
    Result.Size += Alignment.Size;
  }
  return Result;
}

internal void
MemoryArenaClear(gs_memory_arena* Arena)
{
  GS_MEMORY_PROFILE_FUNC;
  
  for (gs_memory_cursor_sll* C = Arena->CursorsRoot;
       C != 0;
       C = C->Next)
  {
    MemoryCursorReset(&C->Cursor);
  }
}

internal void
MemoryArenaFree(gs_memory_arena* Arena)
{
  GS_MEMORY_PROFILE_FUNC;
  
  // NOTE(PS): If this isn't a base arena, we can't
  // really free it.
  // TODO(PS): Once we have the User Specified codepaths
  // in, we can probably provide a way for the user to 
  // let us free a custom allocator
  Assert(Arena->Allocator.PFree);
  
  gs_allocator A = Arena->Allocator;
  gs_memory_cursor_sll* At = Arena->CursorsRoot;
  while (At)
  {
    gs_memory_cursor_sll* NextAt = At->Next;
    
    u64 Size = At->Cursor.Data.Size + sizeof(gs_memory_cursor_sll);
    Free(A, (u8*)At, Size);
    
    At = NextAt;
  }
  
  Arena->CursorsRoot = 0;
  Arena->CursorsHead = 0;
}

#ifdef GS_PLATFORM_IMPLEMENTATION

internal gs_allocator CreatePlatformAllocator();

# if PLATFORM_WINDOWS
#  include "./gs_memory_win32.h"
# elif PLATFORM_OSX
#  include "./gs_memory_osx.h"
# elif PLATFORM_LINUX
#  include "./gs_memory_linux.h"
# endif

#endif

#endif //GS_MEMORY_H
