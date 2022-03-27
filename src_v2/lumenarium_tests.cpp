
Platform_Thread_Result
thread_proc(Platform_Thread_Data* td)
{
  //Sleep(100);
  return {};
}

internal void
run_tests()
{
  // memory tests
  
  u8* a0 = allocator_alloc_array(scratch, u8, 32);
  u8* a1 = allocator_alloc_array(scratch, u8, 32);
  assert(a0 != a1);
  assert((a0 + 32) <= a1);
  
  for (u32 i = 0; i < 32; i++)
  {
    a0[i] = (u8)i;
    a1[i] = (u8)(100 + i);
  }
  
  
  for (u32 i = 0; i < 32; i++)
  {
    assert(a0[i] == i);
    assert(a1[i] == (100 + i));
  }
  
#if defined(PLATFORM_wasm)
  // NOTE(PS): the tests below this point don't make sense on a web assembly
  // platform
  return;
#endif
  
  
  // testing strings and exe path
  String exe_file_path = platform_get_exe_path(scratch);
  assert(exe_file_path.str != 0);
  u64 run_tree_start = string_find_substring(exe_file_path, lit_str("run_tree"), 0, StringMatch_FindLast);
  u64 run_tree_end = run_tree_start + lit_str("run_tree").len;
  assert(run_tree_start < exe_file_path.len);
  String run_tree_path = string_get_prefix(exe_file_path, run_tree_end);
  String run_tree_path_nullterm = string_copy(run_tree_path, scratch);
  assert(run_tree_path_nullterm.len > 0);
  assert(platform_pwd_set(run_tree_path_nullterm));
  
  // testing file io
  Platform_File_Handle f = platform_file_open(lit_str("text.txt"), FileAccess_Read | FileAccess_Write, FileCreate_OpenExisting);
  Platform_File_Info i = platform_file_get_info(f, scratch);
  
  Data d0 = platform_file_read_all(f, scratch);
  assert(d0.size > 0);
  
  String s = lit_str("foooooooooobbbbbbaaaarrrrrr");
  Data d1 = { s.str, s.len };
  bool r = platform_file_write_all(f, d1);
  assert(r);
  
#if 0
  // TODO(PS): these were causing startup problems but you weren't focusing on
  // threads/ When you build something multithreaded come back here and 
  // make tests that actually work
  
  // testing threads
  Platform_Thread_Handle threads[8];
  for (u32 j = 0; j < 8; j++)
  {
    threads[j] = platform_thread_begin(thread_proc, 0);
  }
  
  for (u32 j = 0; j < 8; j++)
  {
    platform_thread_end(threads[j]);
  }
#endif
  
  allocator_clear(scratch);
  
}