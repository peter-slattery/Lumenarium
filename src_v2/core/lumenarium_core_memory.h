#ifndef LUMENARIUM_CORE_MEMORY_H
#define LUMENARIUM_CORE_MEMORY_H

#if !defined(OS_MEM_PAGE_SIZE)
#  error "You must define OS_MEM_PAGE_SIZE for core_memory.h to compile properly"
#endif

//////////////////////////////////////////////
// Memory Helpers

#define Bytes(x) (x)
#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) (((u64)x) << 30)
#define TB(x) (((u64)x) << 40)

#define memory_zero_array(b,t,c) memory_zero((u8*)(b), sizeof(t) * (c))
internal void memory_zero(u8* base, u64 size);
internal void memory_copy(u8* from, u8* to, u64 size);

//////////////////////////////////////////////
//         Data

#define get_byte(value, byte_index) ((value >> (8 * byte_index)) & 0xFF)

typedef struct Data Data;
struct Data
{
  u8* base;
  u64 size;
};

internal Data
data_create(u8* base, u64 size)
{
  Data result = {};
  result.base = base;
  result.size = size;
  return result;
}

//////////////////////////////////////////////
//         Data Writer

typedef struct Data_Writer Data_Writer;
struct Data_Writer
{
  Data data;
  u64 at;
};

// NOTE(PS): functions ending in _b treat data in the Data_Writer as big endian 
// order (network ordering) where functions ending in _l treat data into little 
// endian order
// It is always assumed that values not in the Data_Writer (ie the other args
// to the function or the functions return value) are in little endian order

internal u8   dw_get_u8(Data_Writer* w);
// TODO(PS): get functions

internal void dw_put_u8(Data_Writer* w, u8 b);
internal void dw_put_u16_b(Data_Writer* w, u16 b);
internal void dw_put_u16_l(Data_Writer* w, u16 b);
internal void dw_put_u32_b(Data_Writer* w, u32 b);
internal void dw_put_u32_l(Data_Writer* w, u32 b);
internal void dw_put_u64_b(Data_Writer* w, u64 b);
internal void dw_put_u64_l(Data_Writer* w, u64 b);

//////////////////////////////////////////////
//         Allocator

typedef struct Allocator Allocator;

typedef u8*  Allocator_Alloc(Allocator* allocator, u64 size);
typedef void Allocator_Free(Allocator* allocator, u8* base, u64 size);
typedef u8*  Allocator_Realloc(Allocator* allocator, u8* base, u64 old_size, u64 new_size);
typedef void Allocator_Clear(Allocator* allocator);
typedef void Allocator_Destroy(Allocator* allocator);

struct Allocator
{
  Allocator_Alloc*   alloc;
  Allocator_Free*    free;
  Allocator_Realloc* realloc;
  Allocator_Clear*   clear;
  Allocator_Destroy* destroy;
  
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
#define allocator_destroy(a) (a)->destroy(a)

internal Allocator* paged_allocator_create_reserve(u64 reserve_size, u64 page_size);

//////////////////////////////////////////////
//////////////////////////////////////////////
//         IMPLEMENTATION
//////////////////////////////////////////////
//////////////////////////////////////////////

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

#  define memory_zero_(b,s)   memory_zero_simd((b),(s))
#  define memory_copy_(f,t,s) memory_copy_simd((f),(t),(s))

#else
#  define memory_zero_(b,s)   memory_zero_no_simd((b),(s))
#  define memory_copy_(f,t,s) memory_copy_no_simd((f),(t),(s))

#endif // defined(PLATFORM_HAS_SIMD)

#define zero_struct(s) memory_zero((u8*)(&s), sizeof(s))

internal void memory_zero(u8* base, u64 size) { memory_zero_(base, size); }
internal void memory_copy(u8* from, u8* to, u64 size) { 
  memory_copy_(from, to, size); 
}

// @Maintenance - if the function only has one argument, it should become
// round_to_os_page_multiple
u64
round_size_to_page_multiple(u64 size, u64 page_size)
{
  u64 rem = size % page_size;
  if (rem != 0 || size < page_size)
  {
    u64 grow = page_size - rem;
    size += grow;
  }
  return size;
}

u64
round_size_to_os_page_multiple(u64 size)
{
  return round_size_to_page_multiple(size, OS_MEM_PAGE_SIZE);
}

/////////////////////////////////////////
// Data Writer

internal void 
dw_put_u8(Data_Writer* w, u8 b)
{
  if (w->at < w->data.size)
  {
    w->data.base[w->at++] = b;
  }
}

internal u8
dw_get_u8(Data_Writer* w)
{
  u8 result = 0;
  if (w->at < w->data.size)
  {
    result = w->data.base[w->at];
  }
  return result;
}

internal void
dw_put_u16_b(Data_Writer* w, u16 b)
{
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 0));
}

internal void
dw_put_u16_l(Data_Writer* w, u16 b)
{
  dw_put_u8(w, get_byte(b, 0));
  dw_put_u8(w, get_byte(b, 1));
}

internal void
dw_put_u32_b(Data_Writer* w, u32 b)
{
  dw_put_u8(w, get_byte(b, 3));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 0));
}

internal void
dw_put_u32_l(Data_Writer* w, u32 b)
{
  dw_put_u8(w, get_byte(b, 0));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 3));
}

internal void
dw_put_u64_b(Data_Writer* w, u64 b)
{
  dw_put_u8(w, get_byte(b, 7));
  dw_put_u8(w, get_byte(b, 6));
  dw_put_u8(w, get_byte(b, 5));
  dw_put_u8(w, get_byte(b, 4));
  dw_put_u8(w, get_byte(b, 3));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 0));
}

internal void
dw_put_u64_l(Data_Writer* w, u64 b)
{
  dw_put_u8(w, get_byte(b, 0));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 3));
  dw_put_u8(w, get_byte(b, 4));
  dw_put_u8(w, get_byte(b, 5));
  dw_put_u8(w, get_byte(b, 6));
  dw_put_u8(w, get_byte(b, 7));
}

////////////////////////////////////////
//        Allocator
//
// A generic interface for any memory-providing construct
//
// To implement a complete allocator, all that is really required
// is to create its Allocator_Alloc function

internal void
allocator_destroy_(Allocator* allocator, u64 custom_data_size)
{
  zero_struct(*allocator);
  u64 size = sizeof(Allocator) + custom_data_size;
  os_mem_decommit((u8*)allocator, size);
  os_mem_release((u8*)allocator, size);
}

/////////////////////////////////////////
//        Bump Allocator

typedef struct Allocator_Bump Allocator_Bump;
struct Allocator_Bump
{
  u8* base;
  u64 at;
  u64 size_committed;
  u64 size_reserved;
  u64 page_size;
  u64 high_water_mark;
};

#if defined(DEBUG)
#  define bump_allocator_validate(a) bump_allocator_validate_(a)
#else
#  define bump_allocator_validate(a)
#endif
internal void
bump_allocator_validate_(Allocator* allocator)
{
  Allocator_Bump* bump = (Allocator_Bump*)allocator->allocator_data;
  assert(bump != 0);
  if (bump->size_reserved > 0)
  {
    assert(bump->base != 0);
  }
  assert(bump->at <= bump->size_committed);
  assert(bump->size_committed <= bump->size_reserved);
  assert(bump->page_size > 0);
  assert(bump->high_water_mark <= bump->size_reserved);
}

internal u8*
bump_allocator_alloc_inner(Allocator* allocator, Allocator_Bump* bump, u64 size)
{
  bump_allocator_validate(allocator);
  u64 at_after = bump->at + size;
  // TODO(PS): align up to 8 bytes
  
  if (at_after >= bump->size_committed)
  {
    // determine new size of the arena
    u64 new_size = bump->size_committed * 2;
    if (new_size == 0)
    {
      if (bump->page_size == 0) bump->page_size = OS_MEM_PAGE_SIZE;
      new_size = bump->page_size;
    }
    if (new_size < at_after) 
    {
      new_size = round_size_to_page_multiple(at_after, bump->page_size);
    }
    
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
        u64 next_page = round_size_to_os_page_multiple(bump->at);
        if (bump->at == 0 && bump->size_committed == 0) next_page = 0;
        u64 commit_amt = new_size - next_page;
        u8* new_page = os_mem_commit(bump->base + next_page, commit_amt);
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
  bump->high_water_mark = max(bump->at, bump->high_water_mark);
  
  bump_allocator_validate(allocator);
  return result;
}

internal u8*
bump_allocator_alloc(Allocator* allocator, u64 size)
{
  Allocator_Bump* bump = (Allocator_Bump*)allocator->allocator_data;
  u8* result = bump_allocator_alloc_inner(allocator, bump, size);
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
  bump_allocator_validate(allocator);
}

internal void
bump_allocator_destroy_(Allocator_Bump* bump)
{
  os_mem_decommit(bump->base, bump->size_committed);
  os_mem_release(bump->base, bump->size_reserved);
}

internal void
bump_allocator_destroy(Allocator* allocator)
{
  Allocator_Bump* bump = (Allocator_Bump*)allocator->allocator_data;
  bump_allocator_destroy_(bump);
  allocator_destroy_(allocator, sizeof(Allocator_Bump));
}

internal void
bump_allocator_rewind(Allocator* allocator, u64 to_point)
{
  Allocator_Bump* bump = (Allocator_Bump*)allocator->allocator_data;
  bump_allocator_validate(allocator);
#if defined(DEBUG)
  memory_zero(bump->base + to_point, bump->at - to_point);
#endif
  bump->at = to_point;
  bump_allocator_validate(allocator);
}

internal Allocator*
bump_allocator_create_(u64 page_size)
{
  u64 size_needed = sizeof(Allocator) + sizeof(Allocator_Bump);
  
  u8* base = os_mem_reserve(size_needed);
  base = os_mem_commit(base, size_needed);
  
  Allocator* result = (Allocator*)base;
  zero_struct(*result);
  
  Allocator_Bump* bump = (Allocator_Bump*)base + sizeof(Allocator);
  zero_struct(*bump);
  bump->page_size = page_size;

  result->alloc   = bump_allocator_alloc;
  result->realloc = bump_allocator_realloc;
  result->clear   = bump_allocator_clear;
  result->destroy = bump_allocator_destroy;
  result->allocator_data = (u8*)bump;
  
  bump_allocator_validate(result);
  return result;
}

internal Allocator*
bump_allocator_create_reserve(u64 reserve_size)
{
  Allocator* result = bump_allocator_create_(OS_MEM_PAGE_SIZE);
  Allocator_Bump* bump = (Allocator_Bump*)result->allocator_data;
  
  u64 reserve_pages = round_size_to_os_page_multiple(reserve_size);
  bump->base = os_mem_reserve(reserve_pages);
  if (bump->base != 0) bump->size_reserved = reserve_pages;
  
  bump_allocator_validate(result);
  return result;
}

internal Allocator*
bump_allocator_create_child(Allocator* parent, u64 init_size)
{
  Allocator* result = bump_allocator_create_(OS_MEM_PAGE_SIZE);
  result->parent = parent;
  
  Allocator_Bump* bump = (Allocator_Bump*)result->allocator_data;
  zero_struct(*bump);
  bump->base = allocator_alloc(result->parent, init_size);
  if (bump->base != 0) 
  {
    bump->size_reserved = init_size;
    bump->size_committed = init_size;
  }

  bump_allocator_validate(result);
  return result;
}


/////////////////////////////////////////
//        Scratch Allocator

typedef struct Allocator_Scratch Allocator_Scratch;
struct Allocator_Scratch
{
  Allocator* a;
  u64 at_before;
};

internal Allocator_Scratch
allocator_scratch_begin(Allocator* allocator)
{
	Allocator_Scratch result = {};
	result.a = allocator;
  Allocator_Bump* bump = (Allocator_Bump*)result.a->allocator_data;
  result.at_before = bump->at;
  return result;
}

internal void
allocator_scratch_end(Allocator_Scratch* scratch)
{
	bump_allocator_rewind(scratch->a, scratch->at_before);
	zero_struct(*scratch);
}

/////////////////////////////////////////
//        Paged Allocator

typedef struct Allocator_Paged_Free_Region Allocator_Paged_Free_Region;
struct Allocator_Paged_Free_Region
{
  u64 pages;
  Allocator_Paged_Free_Region* prev;
  Allocator_Paged_Free_Region* next;
};

typedef struct Allocator_Paged Allocator_Paged;
struct Allocator_Paged
{
  Allocator_Bump bump;
  Allocator_Paged_Free_Region* free_first;
};

internal u8*
paged_allocator_alloc(Allocator* allocator, u64 size)
{
  // 1. Find the number of pages we need
  // 2. Find a run of free pages that we can use
  //    If found, 
  //       remove those pages from the run they are in
  //       return those pages of memory
  // 3. Commit pages on the end
  
  Allocator_Paged* paged = (Allocator_Paged*)allocator->allocator_data;
  if (paged->bump.page_size == 0) paged->bump.page_size = OS_MEM_PAGE_SIZE;
  
  u64 rounded_size = round_size_to_page_multiple(size, paged->bump.page_size);
  u64 pages_needed = rounded_size / paged->bump.page_size;
  
  u8* result = 0;
  
  // Find free pages
  if (paged->free_first)
  {
    Allocator_Paged_Free_Region* found = 0;
    for (Allocator_Paged_Free_Region* at = paged->free_first; at != 0; at = at->next)
    {
      // NOTE(PS): this set of conditions checks to see if is bigger than what
      // we need. If it is, we also check to see if this is smaller than any
      // region we've found before. And we abort the search if this region
      // perfectly fits the size needed. 
      //
      // This should make sure that we are always choosing the closest fit we 
      // can. I'm not sure this is the best strategy for dealing with fragmentation
      // but its a decent first pass
      if (at->pages >= pages_needed)
      {
        if (!found || (found->pages > at->pages))
        {
          found = at;
          if (found->pages == pages_needed) break;
        }
      }
    }
    
    if (found)
    {
      result = (u8*)found;
      if (found->pages > pages_needed)
      {
        Allocator_Paged_Free_Region* region_after = (Allocator_Paged_Free_Region*)(result + rounded_size);
        if (found->prev != 0) found->prev->next = region_after;
        region_after = found->next;
      }
      else
      {
        if (found->prev != 0) found->prev->next = found->next;
      }
    }
  }
  
  if (!result)
  {
    result = bump_allocator_alloc_inner(allocator, &paged->bump, size);
  }
  
  return result;
}

#define region_end(r,page_size) ((u8*)(r) + ((r)->pages * page_size))

internal void
paged_region_insert(
                    Allocator_Paged_Free_Region* before, 
                    Allocator_Paged_Free_Region* new_region,  
                    Allocator_Paged_Free_Region* after, 
                    u64 page_size
                    ){
  assert(after == 0 || before < after);
  assert(before < new_region);
  assert(after == 0 || new_region < after);
  assert(new_region->prev == 0 && new_region->next == 0);
  
  u8* before_end = region_end(before, page_size);
  u8* new_region_end = region_end(new_region, page_size);
  
  // Before
  if (before_end == (u8*)new_region)
  {
    // merge the regions
    before->pages += new_region->pages;
    new_region = before;
    assert(new_region_end == region_end(new_region, page_size));
  }
  else
  {
    assert(before_end < (u8*)new_region);
    before->next = new_region;
    new_region->prev = before;
  }
  
  // After
  if (after != 0)
  {
    if (new_region_end == (u8*)after)
    {
      // merge the regions
      new_region->pages += after->pages;
      u8* a = region_end(after, page_size);
      u8* b = region_end(new_region, page_size);
      assert(a == b);
    }
    else
    {
      assert(new_region_end < (u8*)after);
      new_region->next = after;
      after->prev = new_region;
    }
  }
}

internal void
paged_allocator_free(Allocator* allocator, u8* base, u64 size)
{
  // Figure out which page base is the base of, assert its the base
  // figure out how many pages size represents.
  // create a free range
  // stick it in between contiguous free ranges
  // if the ranges before or after meet this new one, merge them all
  
  Allocator_Paged* paged = (Allocator_Paged*)allocator->allocator_data;
  
  u64 page_base_rel = (base - paged->bump.base);
  assert((page_base_rel % paged->bump.page_size) == 0);
  u64 page_index = page_base_rel / paged->bump.page_size;
  u64 size_pages_mult = round_size_to_page_multiple(size, paged->bump.page_size);
  assert((size_pages_mult % paged->bump.page_size) == 0);
  u64 page_count = size_pages_mult / paged->bump.page_size;
  
  Allocator_Paged_Free_Region* region = (Allocator_Paged_Free_Region*)base;
  zero_struct(*region);
  region->pages = page_count;
  
  Allocator_Paged_Free_Region* prev = 0;
  Allocator_Paged_Free_Region* next = 0;
  for (Allocator_Paged_Free_Region* at = paged->free_first; at != 0; at = at->next)
  {
    if (at < region)
    {
      prev = at;
      next = at->next;
      if (next != 0)
      {
        assert(next > region);
        assert((u8*)next >= ((u8*)region + size_pages_mult));
      }
    }
  }
  
  if (prev && next)
  {
    // found a region to insert into
    paged_region_insert(prev, region, next, paged->bump.page_size);
  }
  else if (prev)
  {
    // got to the end and all were before the free region in memory
    paged_region_insert(prev, region, 0, paged->bump.page_size);
  }
  else
  {
    // free list is empty
    paged->free_first = region;
  }
}

internal u8*
paged_allocator_realloc(Allocator* allocator, u8* base, u64 old_size, u64 new_size)
{
  // TODO(PS): 
  // Process:
  // 1. Figure out which page base starts on
  // 2. Find if there is a free region after base that is big enough to house 
  //    the new size
  // 3. If there is a free region, pull the needed memory out of it
  // 4. Otherwise, alloc new_size, copy base into it, and free base
  
  // TODO(PS): you could do a simple version where you just always alloc, copy, free
  return 0;
}

internal void
paged_allocator_clear(Allocator* allocator)
{
  if (!allocator->allocator_data) return;
  Allocator_Paged* paged = (Allocator_Paged*)allocator->allocator_data;
  paged->bump.at = 0;
  paged->free_first = 0;
}

internal void
paged_allocator_destroy(Allocator* allocator)
{
  Allocator_Paged* paged = (Allocator_Paged*)allocator->allocator_data;
  bump_allocator_destroy_(&paged->bump);
  allocator_destroy_(allocator, sizeof(Allocator_Paged));
}

internal Allocator*
paged_allocator_create_()
{
  u64 size_needed = sizeof(Allocator) + sizeof(Allocator_Bump);
  
  u8* base = os_mem_reserve(size_needed);
  base = os_mem_commit(base, size_needed);
  
  Allocator* result = (Allocator*)base;
  zero_struct(*result);
  
  Allocator_Paged* paged = (Allocator_Paged*)base + sizeof(Allocator);
  zero_struct(*paged);
  
  result->alloc   = paged_allocator_alloc;
  result->free    = paged_allocator_free;
  result->realloc = paged_allocator_realloc;
  result->clear   = paged_allocator_clear;
  result->destroy = paged_allocator_destroy;
  result->allocator_data = (u8*)paged;
  
  return result;
}

internal Allocator*
paged_allocator_create_reserve(u64 reserve_size, u64 page_size)
{
  Allocator* result = paged_allocator_create_();
  Allocator_Paged* paged = (Allocator_Paged*)result->allocator_data;
  
  u64 reserve_pages = round_size_to_os_page_multiple(reserve_size);
  paged->bump.page_size = page_size;
  paged->bump.base = os_mem_reserve(reserve_pages);
  if (paged->bump.base != 0) paged->bump.size_reserved = reserve_pages;
  
  return result;
}

internal Allocator*
paged_allocator_create_reserve_os_page_size(u64 reserve_size)
{
  u64 page_size = OS_MEM_PAGE_SIZE;
  return paged_allocator_create_reserve(reserve_size, page_size);
}

internal Allocator*
paged_allocator_create_child(Allocator* parent, u64 init_size)
{
  Allocator* result = bump_allocator_create_(OS_MEM_PAGE_SIZE);
  result->parent = parent;
  
  Allocator_Paged* paged = (Allocator_Paged*)result->allocator_data;
  paged->bump.base = allocator_alloc(result->parent, init_size);
  if (paged->bump.base != 0) 
  {
    paged->bump.size_reserved = init_size;
    paged->bump.size_committed = init_size;
  }
  
  return result;
}

#endif