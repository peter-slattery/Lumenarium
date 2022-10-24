// TODO(PS) @DEPRECATE - new os layer

/* date = April 6th 2022 7:55 pm */

#ifndef LUMENARIUM_MEMORY_H
#define LUMENARIUM_MEMORY_H

/////////////////////////////////////////
//        Allocator

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
#endif //LUMENARIUM_MEMORY_H
