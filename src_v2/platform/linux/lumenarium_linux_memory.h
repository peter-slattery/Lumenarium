#ifndef LUMENARIUM_LINUX_MEMORY_H
#define LUMENARIUM_LINUX_MEMORY_H 1

global u64 linux_page_size_cache_ = 0;

// https://stackoverflow.com/questions/3351940/detecting-the-memory-page-size
u64
os_page_size()
{
  if (linux_page_size_cache_ == 0) 
  {
    long page_size = sysconf(_SC_PAGE_SIZE);
    linux_page_size_cache_ = (u64)page_size;
  }
  return linux_page_size_cache_;
}

size_t
linux_round_to_page_size(uint64_t size)
{
  uint64_t rem = size % linux_page_size_cache_;
  if (rem != 0 || size < linux_page_size_cache_)
  {
    uint64_t grow = linux_page_size_cache_ - rem;
    size += grow;
  }
  return (size_t)size;
}

u8*  
os_mem_reserve(u64 size)
{
  size_t size_cvt = linux_round_to_page_size(size);
  uint8_t* result = (uint8_t*)malloc(size_cvt);
  return result;
}

u8*  
os_mem_commit(u8* base, u64 size)
{
  return base;
}

bool 
os_mem_decommit(u8* base, u64 size)
{
  return 1; // true
}

bool 
os_mem_release(u8* base, u64 size)
{
  free(base);
  return 1; // true
}


#endif // LUMENARIUM_LINUX_MEMORY_H