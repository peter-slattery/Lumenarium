
#define WASM_PAGE_SIZE KB(64)

u64
platform_page_size()
{
  return WASM_PAGE_SIZE;
}

// this comes from: https://surma.dev/things/c-to-webassembly/
extern u8  __heap_base;
global u8* heap_base    = &__heap_base;
global u64 heap_used = 0;

WASM_EXTERN u32 wasm_get_memory_size();
WASM_EXTERN u32 wasm_mem_grow(u32 new_size);

u64
wasm_get_heap_size()
{
  u64 memory_size = wasm_get_memory_size();
  u64 heap_base_addr = (u64)&__heap_base;
  u64 heap_size = memory_size - heap_base_addr;
  return heap_size;
}

u8*
platform_mem_reserve(u64 size)
{
  // if we are out of memory, double the size of wasm memory
  u64 heap_size = wasm_get_heap_size();
  if (heap_used + size >= heap_size)
  {
    u32 heap_size_new = heap_size * 2;
    if (heap_used + size >= heap_size_new) 
    {
      heap_size_new = heap_used + size;
    }
    wasm_mem_grow(heap_size_new);
  }
  assert(heap_used + size <= wasm_get_heap_size());
  
  u64 ptr_addr = (u64)(heap_base + heap_used);
  u8* result = heap_base + heap_used;
  heap_used += size; // TODO(PS): alignment
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
  // TODO(PS): we probably actually want to implement free at some point
  return true;
}

