
/////////////////////////////////////////
//        Memory Functions

void
memory_zero_no_simd(u8* base, u64 size)
{
  for (u64 i = 0; i < size; i++) base[i] = 0;
}

void 
memory_copy_no_simd(u8* from, u8* to, u64 size)
{
  for (u64 i = 0; i < size; i++) to[i] = from[i];
}

#if defined(PLATFORM_HAS_SIMD)

// TODO(PS): 
// TODO(PS): 
// TODO(PS): 

void
memory_zero_simd(u8* base, u64 size)
{
  memory_zero_no_simd(base, size);
}

void
memory_copy_simd(u8* from, u8* to, u64 size)
{
  memory_copy_no_simd(from, to, size);
}

#  define memory_zero(b,s)   memory_zero_simd((b),(s))
#  define memory_copy(f,t,s) memory_copy_simd((f),(t),(s))

#else
#  define memory_zero(b,s)   memory_zero_no_simd((b),(s))
#  define memory_copy(f,t,s) memory_copy_no_simd((f),(t),(s))

#endif // defined(PLATFORM_HAS_SIMD)

#define zero_struct(s) memory_zero((u8*)(&s), sizeof(s))

u64
size_to_pages(u64 size)
{
  u64 page_size = platform_page_size();
  u64 rem = size % page_size;
  if (rem != 0 || size < page_size)
  {
    u64 grow = page_size - rem;
    size += grow;
  }
  return size;
}

/////////////////////////////////////////
//        Allocator
//
// A generic interface for any memory-providing construct
//
// To implement a complete allocator, all that is really required
// is to create its Allocator_Alloc function

typedef struct Allocator Allocator;

typedef u8*  Allocator_Alloc(Allocator* allocator, u64 size);
typedef void Allocator_Free(Allocator* allocator, u8* base, u64 size);
typedef u8*  Allocator_Realloc(Allocator* allocator, u8* base, u64 old_size, u64 new_size);
typedef void Allocator_Clear(Allocator* allocator);

struct Allocator
{
  Allocator_Alloc*   alloc;
  Allocator_Free*    free;
  Allocator_Realloc* realloc;
  Allocator_Clear*   clear;
  
  Allocator* parent;
  
  u8* allocator_data;
};

#define allocator_alloc(a,s) (a)->alloc((a),(s))
#define allocator_alloc_struct(a,t) (t*)(a)->alloc((a),sizeof(t))
#define allocator_alloc_array(a,t,c) (t*)(a)->alloc((a),sizeof(t)*(c))

#define allocator_free(a,b,s) (a)->free((a),(b),(s))
#define allocator_free_struct(a,b,t) (a)->free((a),(b),sizeof(t))
#define allocator_free_array(a,b,t,c) (a)->free((a),(b),sizeof(t)*(c))

#define allocator_realloc(a,b,os,ns) (a)->realloc((a),(b),(os),(ns))
#define allocator_realloc_array(a,b,t,oc,nc) (t*)(a)->realloc((a),(b),sizeof(t)*(oc),sizeof(t)*(nc))

#define allocator_clear(a) (a)->clear(a)

/////////////////////////////////////////
//        Bump Allocator

struct Allocator_Bump
{
  u8* base;
  u64 at;
  u64 size_committed;
  u64 size_reserved;
};

internal u8*
bump_allocator_alloc(Allocator* allocator, u64 size)
{
  Allocator_Bump* bump = (Allocator_Bump*)allocator->allocator_data;
  
  u64 at_after = bump->at + size;
  // TODO(PS): align up to 8 bytes
  
  if (at_after >= bump->size_committed)
  {
    // determine new size of the arena
    u64 new_size = bump->size_committed * 2;
    if (new_size == 0) new_size = platform_page_size();
    if (new_size < at_after) new_size = size_to_pages(at_after);
    
    if (allocator->parent)
    {
      bump->base = allocator_realloc(
                                     allocator->parent, 
                                     bump->base, 
                                     bump->size_committed, 
                                     new_size
                                     );
      if (bump->base != 0) 
      {
        bump->size_reserved = new_size;
        bump->size_committed = new_size;
      }
    }
    else
    {
      if (new_size <= bump->size_reserved)
      {
        u64 next_page = size_to_pages(bump->at);
        if (bump->at == 0 && bump->size_committed == 0) next_page = 0;
        u64 commit_amt = new_size - next_page;
        u8* new_page = platform_mem_commit(bump->base + next_page, commit_amt);
        if (new_page != 0) 
        {
          bump->size_committed = new_size;
        }
      }
      else
      {
        invalid_code_path; // out of reserved memory
      }
    }
  }
  
  u8* result = bump->base + bump->at;
  bump->at = at_after;
  
  return result;
}

internal u8*
bump_allocator_realloc(Allocator* allocator, u8* base, u64 old_size, u64 new_size)
{
  u8* result = bump_allocator_alloc(allocator, new_size);
  memory_copy(base, result, old_size);
  return result;
}

internal void
bump_allocator_clear(Allocator* allocator)
{
  if (!allocator->allocator_data) return;
  Allocator_Bump* bump = (Allocator_Bump*)allocator->allocator_data;
  bump->at = 0;
}

internal Allocator*
bump_allocator_create_()
{
  u64 size_needed = sizeof(Allocator) + sizeof(Allocator_Bump);
  
  u8* base = platform_mem_reserve(size_needed);
  base = platform_mem_commit(base, size_needed);
  
  Allocator* result = (Allocator*)base;
  zero_struct(*result);
  
  Allocator_Bump* bump = (Allocator_Bump*)base + sizeof(Allocator);
  zero_struct(*bump);
  
  result->alloc   = bump_allocator_alloc;
  result->realloc = bump_allocator_realloc;
  result->clear   = bump_allocator_clear;
  result->allocator_data = (u8*)bump;
  
  return result;
}

internal Allocator*
bump_allocator_create_reserve(u64 reserve_size)
{
  Allocator* result = bump_allocator_create_();
  Allocator_Bump* bump = (Allocator_Bump*)result->allocator_data;
  
  u64 reserve_pages = size_to_pages(reserve_size);
  bump->base = platform_mem_reserve(reserve_pages);
  if (bump->base != 0) bump->size_reserved = reserve_pages;
  
  return result;
}

internal Allocator*
bump_allocator_create_child(Allocator* parent, u64 init_size)
{
  Allocator* result = bump_allocator_create_();
  result->parent = parent;
  
  Allocator_Bump* bump = (Allocator_Bump*)result->allocator_data;
  bump->base = allocator_alloc(result->parent, init_size);
  if (bump->base != 0) 
  {
    bump->size_reserved = init_size;
    bump->size_committed = init_size;
  }
  
  return result;
}
