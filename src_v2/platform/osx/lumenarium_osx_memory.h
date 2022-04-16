// TODO(PS): come back and do this properly
#define OS_MEM_PAGE_SIZE 4096

size_t
osx_round_to_page_size(uint64_t size)
{
  uint64_t rem = size % OS_MEM_PAGE_SIZE;
  if (rem != 0 || size < OS_MEM_PAGE_SIZE)
  {
    uint64_t grow = OS_MEM_PAGE_SIZE - rem;
    size += grow;
  }
  return (size_t)size;
}

uint8_t*  
os_mem_reserve(uint64_t size)
{
  size_t size_cvt = osx_round_to_page_size(size);
  uint8_t* result = (uint8_t*)malloc(size_cvt);
  return result;
}

uint8_t*
os_mem_commit(uint8_t* base, uint64_t size)
{
  return base;
}

bool
os_mem_decommit(uint8_t* base, uint64_t size)
{
  return 1; // true
}

bool
os_mem_release(uint8_t* base, uint64_t size)
{
  free(base);
  return 1; // true
}