internal u32
NextRandom(u32* LastRandomValue)
{
  u32 Result = *LastRandomValue;
  Result ^= Result << 13;
  Result ^= Result >> 17;
  Result ^= Result << 5;
  *LastRandomValue = Result;
  return Result;
}

void
MemoryArenaTests()
{
  Test("Allocator")
  {
    gs_allocator A = CreatePlatformAllocator();
    
    u8* Data = AllocArray(A, u8, 4096, "root");
    for (int i = 0; i < 4096; i++) Data[i] = (i % MaxU8);
    bool Success = true;
    for (int i = 0; i < 4096; i++) Success &= Data[i] == (i % MaxU8);
    TestResult(Success);
    
    FreeArray(A, Data, u8, 4096);
    TestResult(true); // TODO(PS): How do we test free?
  }
  
  Test("Memory Cursor")
  {
    gs_allocator A = CreatePlatformAllocator();
    
    u64 Size = 4096;
    gs_data D = AllocData(A, Size, "root");
    gs_memory_cursor C = MemoryCursorCreate(D.Memory, D.Size);
    
    u64 RoomLeft = MemoryCursorRoomLeft(C);
    TestResult(RoomLeft == Size);
    TestResult(MemoryCursorHasRoom(C));
    
    TestResult(MemoryCursorCanPush(C, 2048));
    TestResult(MemoryCursorCanPush(C, Size));
    TestResult(!MemoryCursorCanPush(C, Size + 1));
    
    for (u64 i = 0; i < 2048; i++)
    {
      u8* Byte = MemoryCursorPushSize(&C, 1).Memory;
      *Byte = (u8)(i % 256);
    }
    RoomLeft = MemoryCursorRoomLeft(C);
    TestResult(RoomLeft == (Size - 2048));
    
    MemoryCursorPopSize(&C, 2048);
    TestResult(C.Position == 0);
    
    bool Success = true;
    for (u64 i = 0; i < 2048; i++)
    {
      u8* Byte = MemoryCursorPushSize(&C, 1).Memory;
      Success &= *Byte == (u8)(i % 256);
    }
    TestResult(Success);
  }
  
  Test("Memory Arena")
  {
    gs_allocator Al = CreatePlatformAllocator();
    gs_memory_arena A = MemoryArenaCreate(128, 4, Al, 0, 0, "Test");
    
    // NOTE(PS): We loop through this block 3 times
    // 1. Make sure the arena works out of the box
    // 2. Make sure the arena works the same way after clearing
    // 3. Make sure the arena works the same way after freeing
    for (int i = 0; i < 3; i++)
    {
      gs_data D0 = PushSize_(&A, 32, DEBUG_LOC);
      TestResult(D0.Size == 32);
      
      // NOTE(PS): This should still result in 32 bytes
      // because its going to align the Cursor after 
      // it allocates to a multiple of 4 bytes
      gs_data D1 = PushSize_(&A, 30, DEBUG_LOC);
      TestResult(D1.Size == 32);
      
      // NOTE(PS): Allocating bigger than the size remaining
      // in the current cursor
      gs_data D2 = PushSize_(&A, 128, DEBUG_LOC);
      TestResult(D2.Size == 128);
      TestResult(A.CursorsRoot != A.CursorsHead);
      
      // NOTE(PS): Because there is still room in cursor
      // 0, the head of this gs_data should be one byte
      // past the end of D1
      gs_data D3 = PushSize_(&A, 32, DEBUG_LOC);
      TestResult(D3.Memory == D1.Memory + D1.Size);
      
      if (i == 0) 
      {
        MemoryArenaClear(&A);
      } else if (i == 1) {
        MemoryArenaFree(&A);
      }
    }
  }
  
  Test("Memory Arena - Push")
  {
    gs_allocator Al = CreatePlatformAllocator();
    gs_memory_arena A = MemoryArenaCreate(128, 8, Al, 0, 0, "Test");
    
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
    
  }
  
  int FreeCount = 0;
  int ClearCount = 0;
  
  gs_debug_allocations_list* DEBUGAllocations = 0;
  
  Test("Memory Arena - Stress Test")
  {
    // NOTE(PS): We're going to create thousands of allocations
    // on the allocator of varying sizes. We're also going to clear
    // and free the arena at random times to make sure it all works.
    
    gs_allocator Al = CreatePlatformAllocator();
    gs_memory_arena A = MemoryArenaCreate(4096, 4, Al, 0, 0, "Test");
    
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
      NextRandom(&RandomSeed);
      u32 SizeIndex = RandomSeed % RandomSizesCount;
      u64 RandomSize = RandomSizes[SizeIndex];
      
      if (RandomSize == 0)
      {
        MemoryArenaClear(&A);
        ClearCount++;
      } else if (RandomSize == 2) {
        MemoryArenaFree(&A);
        FreeCount++;
      } else {
        gs_data D = PushSize_(&A, RandomSize, DEBUG_LOC);
        // NOTE(PS): This check has to be >= because the arena
        // might have adjusted to maintain alignment on this 
        // allocation.
        Success &= D.Size >= RandomSize;
      }
    }
    
    TestResult(Success);
    
    DEBUGAllocations = Al.DEBUGAllocList;
  }
  
  printf("\tMemory Arena Cleared: %d times\n", ClearCount);
  printf("\tMemory Arena Freed:   %d times\n", FreeCount);
  
#if 0
  printf("\n\nAllocations:\n");
  for (gs_debug_memory_allocation* ARecord = DEBUGAllocations->Root;
       ARecord != 0;
       ARecord = ARecord->Next)
  {
    printf("\t");
    printf("%lld\t%s:%d - %s\n", 
           ARecord->Size,
           ARecord->Loc.File,
           ARecord->Loc.Line,
           ARecord->Loc.Function);
  }
#endif
}