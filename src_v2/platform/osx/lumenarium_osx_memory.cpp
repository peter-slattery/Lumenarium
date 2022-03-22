#define OSX_PAGE_SIZE KB(4) // TODO(PS): look this up

u64 platform_page_size() { return OSX_PAGE_SIZE; }

u8*  
platform_mem_reserve(u64 size)
{
  size_t size_cvt = (size_t)size_to_pages(size);
  u8* result = (u8*)malloc(size);
  return result;
}

u8*
platform_mem_commit(u8* base, u64 size)
{
  return base;
}

bool
platform_mem_decommit(u8* base, u64 size)
{
  return true;
}

bool
platform_mem_release(u8* base, u64 size)
{
  free(base);
  return true;
}
