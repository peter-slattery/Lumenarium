#define WIN32_PAGE_SIZE KB(4)

u64 platform_page_size() { return WIN32_PAGE_SIZE; }

u8*  
platform_mem_reserve(u64 size)
{
  size_t size_cvt = (size_t)size_to_pages(size);
  DWORD alloc_type = MEM_RESERVE;
  DWORD protection = PAGE_READWRITE;
  u8* result = (u8*)VirtualAlloc(0, size_cvt, alloc_type, protection);
  if (!result) win32_get_last_error();
  return result;
}

u8*
platform_mem_commit(u8* base, u64 size)
{
  size_t size_cvt = (size_t)size_to_pages(size);
  DWORD alloc_type = MEM_COMMIT;
  DWORD protection = PAGE_READWRITE;
  u8* result = (u8*)VirtualAlloc(base, size_cvt, alloc_type, protection);
  if (!result) win32_get_last_error();
  return result;
}

bool
platform_mem_decommit(u8* base, u64 size)
{
  DWORD free_type = MEM_DECOMMIT;
  bool result = VirtualFree(base, (size_t)size, free_type);
  if (!result) win32_get_last_error();
  return result;
}

bool
platform_mem_release(u8* base, u64 size)
{
  DWORD free_type = MEM_RELEASE;
  bool result = VirtualFree(base, size, free_type);
  if (!result) win32_get_last_error();
  return result;
}
