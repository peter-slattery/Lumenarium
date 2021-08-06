#include "../app/platform_win32/win32_foldhaus_memory.h"

internal u32
TESTNextRandom(u32* LastRandomValue)
{
  u32 Result = *LastRandomValue;
  Result ^= Result << 13;
  Result ^= Result >> 17;
  Result ^= Result << 5;
  *LastRandomValue = Result;
  return Result;
}

internal void
MemoryArenaTests()
{
  Test("Allocator")
  {
    gs_allocator Allocator = CreateAllocator(Win32Alloc, Win32Free);
    
    u8* Data = AllocatorAllocArray(Allocator, u8, 4096);
    for (int i = 0; i < 4096; i++) Data[i] = (i % MaxU8);
    bool Success = true;
    for (int i = 0; i < 4096; i++) Success &= (Data[i] == (i % MaxU8));
    TestResult(Success);
    
    AllocatorFreeArray(Allocator, Data, u8, 4096);
    // idk how to test free
  }
  
  Test("Memory Cursor")
  {
    gs_allocator A = CreateAllocator(Win32Alloc, Win32Free);
    
    u64 Size = 4096;
    gs_data D = AllocatorAlloc(A, Size);
    gs_memory_cursor C = CreateMemoryCursor(D);
    
    u64 RoomLeft = CursorRoomLeft(C);
    TestResult(RoomLeft == Size);
    
    TestResult(CursorHasRoom(C, 2048));
    TestResult(CursorHasRoom(C, Size));
    TestResult(!CursorHasRoom(C, Size + 1));
    
    for (u64 i = 0; i < 2048; i++)
    {
      u8* Byte = PushSizeOnCursor(&C, 1).Memory;
      *Byte = (u8)(i % 256);
    }
    RoomLeft = CursorRoomLeft(C);
    TestResult(RoomLeft == (Size - 2048));
    
    PopSizeOnCursor(&C, 2048);
    TestResult(C.Position == 0);
    
    bool Success = true;
    for (u64 i = 0; i < 2048; i++)
    {
      u8* Byte = PushSizeOnCursor(&C, 1).Memory;
      Success &= *Byte == (u8)(i % 256);
    }
    TestResult(Success);
    
    AllocatorFree(A, D.Memory, D.Size);
  }
  
  Test("Memory Arena")
  {
    gs_allocator Al = CreateAllocator(Win32Alloc, Win32Free);
    gs_memory_arena A = CreateMemoryArena(Al, "test", 128, 4);
    
    // NOTE(PS): We loop through this block 3 times
    // 1. Make sure the arena works out of the box
    // 2. Make sure the arena works the same way after clearing
    // 3. Make sure the arena works the same way after freeing
    for (int i = 0; i < 3; i++)
    {
      gs_data D0 = PushSize_(&A, 32, FileNameAndLineNumberString);
      TestResult(D0.Size == 32);
      
      // NOTE(PS): This should still result in 32 bytes
      // because its going to align the Cursor after 
      // it allocates to a multiple of 4 bytes
      gs_data D1 = PushSize_(&A, 30, FileNameAndLineNumberString);
      TestResult(D1.Size == 32);
      
      // NOTE(PS): Allocating bigger than the size remaining
      // in the current cursor
      gs_data D2 = PushSize_(&A, 128, FileNameAndLineNumberString);
      TestResult(D2.Size == 128);
      TestResult(A.CursorsCount != 1);
      
      // NOTE(PS): Because there is still room in cursor
      // 0, the head of this gs_data should be one byte
      // past the end of D1
      gs_data D3 = PushSize_(&A, 32, FileNameAndLineNumberString);
      TestResult(D3.Memory == D1.Memory + D1.Size);
      
      if (i == 0) 
      {
        ClearArena(&A);
      } else if (i == 1) {
        FreeMemoryArena(&A);
      }
    }
    
    FreeMemoryArena(&A);
  }
  
  Test("Memory Arena: Push")
  {
    gs_allocator Al = CreateAllocator(Win32Alloc, Win32Free);
    gs_memory_arena A = CreateMemoryArena(Al, "test", 128, 4);
    
    // NOTE(PS): This makes sure that the Arena is moving its next allocation
    // pointer forward the appropriate amount after each allocation. If it isnt'
    // then Array1 should be overlapping with Array0 in the event that the arena
    // doesn't push the pointer forward enough
    u32* Array0 = PushArray(&A, u32, 32);
    u32* Array1 = PushArray(&A, u32, 32);
    
    for (u32 i = 0; i < 32; i++)
    {
      Array0[i] = i;
      Array1[i] = i * 4;
    }
    
    bool Success = true;
    for (u32 i = 0; i < 32; i++)
    {
      Success &= Array0[i] == i && Array1[i] == i * 4;
    }
    TestResult(Success);
    
    FreeArena(&A);
  }
  
  int FreeCount = 0;
  int ClearCount = 0;
  
  Test("Memory Arena: Stress Test")
  {
    gs_allocator Al = CreateAllocator(Win32Alloc, Win32Free);
    gs_memory_arena A = CreateMemoryArena(Al, "test", 128, 4);
    
    // NOTE(PS): This is an array of allocation sizes
    // As we repeat the loop we will get values out of this array 
    // semi-randomly.
    // * if the value is 0, we will clear the arena
    // * if the value is 2, we will free the arena
    // * otherwise we will push a value sized allocation on the arena
    u64 RandomSizes[] = { 8, 32, 128, 93, 1256, 4098, 0, 1024, 7, 18, 967, 53, 1, 2 };
    u32 RandomSizesCount = sizeof(RandomSizes) / sizeof(u64);
    
    bool Success = true;
    u32 RandomSeed = 1923;
    for (u64 i = 0; i < (4096 * 14); i++)
    {
      TESTNextRandom(&RandomSeed);
      u32 SizeIndex = RandomSeed % RandomSizesCount;
      u64 RandomSize = RandomSizes[SizeIndex];
      
      if (RandomSize == 0)
      {
        ClearArena(&A);
        ClearCount++;
      } else if (RandomSize == 2) {
        FreeArena(&A);
        FreeCount++;
      } else {
        gs_data D = PushSize_(&A, RandomSize, FileNameAndLineNumberString);
        // NOTE(PS): This check has to be >= because the arena
        // might have adjusted to maintain alignment on this 
        // allocation.
        Success &= D.Size >= RandomSize;
      }
    }
    
    TestResult(Success);
    
  }
  
}
