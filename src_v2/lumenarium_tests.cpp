Thread_Result
thread_proc(Thread_Data* td)
{
  //Sleep(100);
  return (Thread_Result){};
}

void
memory_allocator_tests(Allocator* a, bool run_free_tests)
{
  // TestGroup("Allocator Push")
  {
    for (uint32_t i = 0; i < 3; i++)
    {
      uint8_t* buf0 = allocator_alloc(a, 256);
      buf0[0] = 200;
      buf0[255] = 199;
      assert(buf0[0] == 200);
      assert(buf0[255] == 199);
      
      uint8_t* buf1 = allocator_alloc(a, 256);
      buf1[0] = 201;
      buf1[255] = 202;
      assert(buf1 >= (buf0 + 256));
      assert(buf0[0] == 200);
      assert(buf0[255] == 199);
      assert(buf1[0] == 201);
      assert(buf1[255] == 202);
      
      allocator_clear(a);
    }
  }
  
  // TestGroup("Allocator Free")
  if (run_free_tests)
  {
    for (uint32_t i = 0; i < 3; i++)
    {
      uint8_t* buf0 = allocator_alloc(a, KB(4));
      uint8_t* buf1 = allocator_alloc(a, KB(4));
      uint8_t* buf2 = allocator_alloc(a, KB(4));
      uint8_t* buf3 = allocator_alloc(a, KB(4));
      uint8_t* buf4 = allocator_alloc(a, KB(4));
      assert((buf1 - buf0) >= KB(4));
      assert((buf2 - buf0) >= KB(8));
      assert((buf3 - buf0) >= KB(12));
      assert((buf4 - buf0) >= KB(16));
      
      allocator_free(a, buf1, KB(4));
      allocator_free(a, buf2, KB(4));
      uint8_t* buf5 = allocator_alloc(a, KB(7));
      // buf5 should get put in the place of buf1 since buf1 and 2 get
      // merged
      assert(buf5 == buf1);
      
      allocator_free(a, buf4, KB(4));
      allocator_free(a, buf3, KB(4));
      allocator_free(a, buf0, KB(4));
      uint8_t* buf6 = allocator_alloc(a, KB(4));
      assert(buf0 == buf6);
      
      allocator_clear(a);
    }
  }
}

void
memory_tests()
{
  // TestGroup("Platform Allocation")
  {
    u64 size = GB(32);
#if defined(PLATFORM_wasm)
    size = KB(4);
#elif defined(PLATFORM_raspi)
    size = KB(32);
#endif
    
    uint8_t* base = os_mem_reserve(size);
    os_mem_commit(base, KB(4));
    base[4095] = 200;
    assert(base[4095] == 200);
    os_mem_commit(base + KB(4), KB(4));
    base[5000] = 200;
    assert(base[5000] == 200);
    os_mem_decommit(base, KB(8));
    os_mem_release(base, size);
  }
  
  Allocator* bump = bump_allocator_create_reserve(KB(32));
  memory_allocator_tests(bump, false);
  allocator_destroy(bump);
  
  Allocator* paged = paged_allocator_create_reserve(KB(32), KB(4));
  memory_allocator_tests(paged, true);
  allocator_destroy(paged);
}

enum test_flags
{
  TestNone = 0,
  Test1 = 1,
  Test2 = 2,
  Test3 = 4,
  Test4 = 8,
};

static void
run_tests()
{
  scratch_get(scratch);
  
  // basic 
  
  uint8_t b = TestNone;
  assert(!has_flag(b, TestNone));
  assert(!has_flag(b, Test1));
  add_flag(b, Test1);
  assert(has_flag(b, Test1));
  assert(!has_flag(b, Test2));
  add_flag(b, Test2);
  assert(has_flag(b, Test1));
  assert(has_flag(b, Test2));
  assert(has_flag(b, Test1 | Test2));
  add_flag(b, Test4);
  assert(has_flag(b, Test1));
  assert(has_flag(b, Test2));
  assert(has_flag(b, Test4));
  assert(has_flag(b, Test1 | Test2 | Test4));
  assert(!has_flag(b, Test3));
  rem_flag(b, Test2);
  assert(has_flag(b, Test1));
  assert(!has_flag(b, Test2));
  assert(has_flag(b, Test4));
  assert(has_flag(b, Test1 | Test4));
  assert(!has_flag(b, Test3));

  // memory tests
  uint8_t* r0 = os_mem_reserve(1024);
  uint8_t* r1 = os_mem_commit(r0, 512);
  for (uint32_t i = 0; i < 512; i++) r1[i] = i;
  os_mem_decommit(r1, 512);
  os_mem_release(r0, 1024);
  // r0[256] = 100; // this should break if you uncomment

  uint8_t* a0 = allocator_alloc_array(scratch.a, uint8_t, 32);
  uint8_t* a1 = allocator_alloc_array(scratch.a, uint8_t, 32);
  assert(a0 != a1);
  assert((a0 + 32) <= a1);

  a1[0] = 25;
  
  for (uint32_t i = 0; i < 32; i++)
  {
    a0[i] = (uint8_t)i;
    a1[i] = (uint8_t)(100 + i);
  }
  
  
  for (uint32_t i = 0; i < 32; i++)
  {
    assert(a0[i] == i);
    assert(a1[i] == (100 + i));
  }
  

  assert(round_up_to_pow2_u32(1) == 1);
  assert(round_up_to_pow2_u32(3) == 4);
  assert(round_up_to_pow2_u32(29) == 32);
  assert(round_up_to_pow2_u32(32) == 32);
  assert(round_up_to_pow2_u32(120) == 128);
  

  memory_tests();
  bsp_tests();
  
#if defined(PLATFORM_wasm)
  // NOTE(PS): the tests below this point don't make sense on a web assembly
  // platform
  return;
#endif
  
  
  // testing strings and exe path
  String exe_file_path = os_get_exe_path(scratch.a);
  assert(exe_file_path.str != 0);
  u64 run_tree_start = string_find_substring(exe_file_path, lit_str("run_tree"), 0, StringMatch_FindLast);
  u64 run_tree_end = run_tree_start + lit_str("run_tree").len;
  assert(run_tree_start < exe_file_path.len);
  String run_tree_path = string_get_prefix(exe_file_path, run_tree_end);
  String run_tree_path_nullterm = string_copy(run_tree_path, scratch.a);
  assert(run_tree_path_nullterm.len > 0);
  assert(os_pwd_set(run_tree_path_nullterm));
  

  // testing file io
  File_Handle f = os_file_open(lit_str("text.txt"), FileAccess_Read | FileAccess_Write, FileCreate_OpenExisting);
  File_Info i = os_file_get_info(f, scratch.a);
  
  Data d0 = os_file_read_all(f, scratch.a);
  assert(d0.size > 0);
  
  String s = lit_str("foooooooooobbbbbbaaaarrrrrr");
  Data d1 = { s.str, s.len };
  bool r = os_file_write_all(f, d1);
  assert(r);

#if 0
  // TODO(PS): these were causing startup problems but you weren't focusing on
  // threads/ When you build something multithreaded come back here and 
  // make tests that actually work
  
  // testing threads
  Platform_Thread_Handle threads[8];
  for (uint32_t j = 0; j < 8; j++)
  {
    threads[j] = platform_thread_begin(thread_proc, 0);
  }
  
  for (uint32_t j = 0; j < 8; j++)
  {
    platform_thread_end(threads[j]);
  }
#endif
  scratch_release(scratch);
}