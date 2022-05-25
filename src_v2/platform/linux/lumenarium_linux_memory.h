#ifndef LUMENARIUM_LINUX_MEMORY_H
#define LUMENARIUM_LINUX_MEMORY_H 1

static uint64_t linux_page_size_cache_ = 0;

// https://stackoverflow.com/questions/3351940/detecting-the-memory-page-size
uint64_t
os_page_size()
{
  if (linux_page_size_cache_ == 0) 
  {
    long page_size = sysconf(_SC_PAGE_SIZE);
    linux_page_size_cache_ = (uint64_t)page_size;
  }
  return linux_page_size_cache_;
}

#define OS_MEM_PAGE_SIZE os_page_size()

#ifdef DEBUG
#  define linux_memory_assert_always (*((volatile int*)0) = 0xFFFF)
#  define linux_memory_assert(c) do { \
  if (!(c)) { \
    linux_memory_assert_always;\
  }} while(false)
#endif // DEBUG

#define SIZE_T_MAX_U64 (uint64_t)((size_t)0 - 1)

size_t
linux_round_to_page_size(uint64_t size)
{
  uint64_t m = SIZE_T_MAX_U64;
  linux_memory_assert(size < SIZE_T_MAX_U64);
  uint64_t rem = size % os_page_size();
  if (rem != 0 || size < os_page_size())
  {
    uint64_t grow = os_page_size() - rem;
    size += grow;
  }
  return (size_t)size;
}

uint8_t*  
os_mem_reserve(uint64_t size)
{
  size_t size_cvt = linux_round_to_page_size(size);
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


#endif // LUMENARIUM_LINUX_MEMORY_H