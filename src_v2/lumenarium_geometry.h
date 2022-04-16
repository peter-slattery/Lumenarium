/* date = April 11th 2022 3:22 pm */

#ifndef LUMENARIUM_GEOMETRY_H
#define LUMENARIUM_GEOMETRY_H

// Utility functions for working with 3d geometry

// NOTE(PS): the Buffer vs Buffer_Builder distinction
// lets a static sized buffer exist, and be auto 'exported' or 'available'
// from a larger data structure that also stores enough data to incrementally
// build the buffer

// NOTE(PS): @IMPORTANT!!!
// at the moment this builder doesn't support allowing for the elements
// of a vertex to be in an order other than position tex color
// and in any other packing than tightly packed
typedef u32 Geo_Vertex_Buffer_Storage;
enum
{
  GeoVertexBufferStorage_None     = 0,
  GeoVertexBufferStorage_Position = 1,
  GeoVertexBufferStorage_TexCoord = 2,
  GeoVertexBufferStorage_Color    = 4
};

typedef struct Geo_Vertex_Buffer Geo_Vertex_Buffer;
struct Geo_Vertex_Buffer
{
  r32* values;
  u32 len;
  Geo_Vertex_Buffer_Storage storage;
  u32 stride;
};

typedef struct Geo_Vertex_Buffer_Builder Geo_Vertex_Buffer_Builder;
struct Geo_Vertex_Buffer_Builder
{
  union
  {
    Geo_Vertex_Buffer buffer;
    struct 
    {
      // NOTE(PS): @Maintainence
      // this must always match the layout of Geo_Vertex_Buffer
      r32* values;
      u32 len;
      Geo_Vertex_Buffer_Storage storage;
      u32 stride;
    };
  };
  u32 cap;
};

typedef struct Geo_Index_Buffer Geo_Index_Buffer;
struct Geo_Index_Buffer
{
  u32* values;
  u32 len;
};

typedef struct Geo_Index_Buffer_Builder Geo_Index_Buffer_Builder;
struct Geo_Index_Buffer_Builder
{
  union
  {
    Geo_Index_Buffer buffer;
    struct
    {
      // NOTE(PS): @Maintainence
      // this must always match the layout of Geo_Index_Buffer
      u32* values;
      u32 len;
    };
  };
  u32 cap;
};

typedef struct Geo_Quad_Buffer Geo_Quad_Buffer;
struct Geo_Quad_Buffer
{
  Geo_Vertex_Buffer buffer_vertex;
  Geo_Index_Buffer  buffer_index;
};

typedef struct Geo_Quad_Buffer_Builder Geo_Quad_Buffer_Builder;
struct Geo_Quad_Buffer_Builder
{
  Geo_Vertex_Buffer_Builder buffer_vertex;
  Geo_Index_Buffer_Builder  buffer_index;
};

internal Geo_Vertex_Buffer_Builder geo_vertex_buffer_builder_create(Allocator* a, u32 cap, Geo_Vertex_Buffer_Storage storage);
internal Geo_Index_Buffer_Builder  geo_index_buffer_builder_create(Allocator* a, u32 cap);
internal Geo_Quad_Buffer_Builder   ggeo_quad_buffer_builder_create(Allocator* a, u32 vertex_cap, Geo_Vertex_Buffer_Storage storage, u32 index_cap);

// Vertex Buffer
internal u32 geo_vertex_buffer_builder_push_vtc(Geo_Vertex_Buffer_Builder* b, v3 v, v2 t, v4 c);
internal u32 geo_vertex_buffer_builder_push_vt(Geo_Vertex_Buffer_Builder* b, v3 v, v2 t);
internal u32 geo_vertex_buffer_builder_push_v(Geo_Vertex_Buffer_Builder* b, v3 v);

// Index Buffer
internal u32  geo_index_buffer_builder_push(Geo_Index_Buffer_Builder* b, u32 i);
internal void geo_index_buffer_builder_push_tri(Geo_Index_Buffer_Builder* b, u32 i0, u32 i1, u32 i2);
internal void geo_index_buffer_builder_push_quad(Geo_Index_Buffer_Builder* b, u32 i0, u32 i1, u32 i2, u32 i3);

// Quad Buffer
internal void geo_quad_buffer_builder_push_vtc(Geo_Quad_Buffer_Builder* b, v3 p0, v3 p1, v3 p2, v3 p3, v2 t0, v2 t1, v2 t2, v2 t3, v4 c);
internal void geo_quad_buffer_builder_push_vt(Geo_Quad_Buffer_Builder* b, v3 p0, v3 p1, v3 p2, v3 p3, v2 t0, v2 t1, v2 t2, v2 t3);
internal void geo_quad_buffer_builder_push_v(Geo_Quad_Buffer_Builder* b, v3 p0, v3 p1, v3 p2, v3 p3);
internal Geo_Quad_Buffer geo_quad_buffer_builder_get_static_buffer(Geo_Quad_Buffer_Builder* b);

/////////////////////////////////////////////
// Implementation

internal u32
geo_vertex_buffer_builder_stride(Geo_Vertex_Buffer_Storage storage)
{
  u32 result = 0;
  result += has_flag(storage, GeoVertexBufferStorage_Position) ? 3 : 0;
  result += has_flag(storage, GeoVertexBufferStorage_TexCoord) ? 2 : 0;
  result += has_flag(storage, GeoVertexBufferStorage_Color)    ? 4 : 0;
  return result;
}

#ifdef DEBUG
#  define geo_vertex_buffers_validate(g) geo_vertex_buffers_validate_(g) 
#else
#  define geo_vertex_buffers_validate(g) 
#endif

void 
geo_vertex_buffers_validate_(Geo_Vertex_Buffer_Builder* b)
{
  // before start
  u32* sentinel = (u32*)(b->values - 2);
  assert(sentinel[0] == 0xF0F0F0F0);
  assert(sentinel[1] == 0x0F0F0F0F);

  // after end
  sentinel = (u32*)(b->values + (b->cap * b->stride));
  assert(sentinel[0] == 0xFFFF000F);
  assert(sentinel[1] == 0xF000FFFF);
}

internal Geo_Vertex_Buffer_Builder
geo_vertex_buffer_builder_create(Allocator* a, u32 cap, Geo_Vertex_Buffer_Storage storage)
{
  u32 stride = geo_vertex_buffer_builder_stride(storage);
  u32 size = cap * stride;
  u32 alloc_size = size;
#ifdef DEBUG
  alloc_size += 4;
#endif
  
  Geo_Vertex_Buffer_Builder result = {};
  zero_struct(result);
  result.cap = cap;
  result.storage = storage;
  result.stride = stride;
  result.values = allocator_alloc_array(a, r32, alloc_size);

#ifdef DEBUG
  
  u32* sentinel = (u32*)result.values;
  sentinel[0] = 0xF0F0F0F0;
  sentinel[1] = 0x0F0F0F0F;
  sentinel = (u32*)(result.values + alloc_size - 2);
  result.values += 2;
  sentinel[0] = 0xFFFF000F;
  sentinel[1] = 0xF000FFFF;
#endif
  
  geo_vertex_buffers_validate(&result);
  return result;
}

internal Geo_Index_Buffer_Builder
geo_index_buffer_builder_create(Allocator* a, u32 cap)
{
  Geo_Index_Buffer_Builder result = {};
  zero_struct(result);
  result.cap = cap;
  result.values = allocator_alloc_array(a, u32, cap);
  return result;
}

internal Geo_Quad_Buffer_Builder
geo_quad_buffer_builder_create(Allocator* a, u32 vertex_cap, Geo_Vertex_Buffer_Storage storage, u32 index_cap)
{
  Geo_Quad_Buffer_Builder result = {};
  result.buffer_vertex = geo_vertex_buffer_builder_create(a, vertex_cap, storage);
  result.buffer_index = geo_index_buffer_builder_create(a, index_cap);
  return result;
}

// Vertex Buffer

internal u32
geo_vertex_buffer_builder_push_vtc(Geo_Vertex_Buffer_Builder* b, v3 v, v2 t, v4 c)
{
  geo_vertex_buffers_validate(b);
  assert(b->len < b->cap);
  u32 result = b->len++;
  u32 offset = result * b->stride;
  assert((offset + b->stride) <= (b->cap * b->stride));
  r32* at = b->values + offset;
  if (has_flag(b->storage, GeoVertexBufferStorage_Position))
  {
    *at++ = v.x;
    *at++ = v.y;
    *at++ = v.z;
  }
  if (has_flag(b->storage, GeoVertexBufferStorage_TexCoord))
  {
    *at++ = t.x;
    *at++ = t.y;
  }
  if (has_flag(b->storage, GeoVertexBufferStorage_Color))
  {
    *at++ = c.x;
    *at++ = c.y;
    *at++ = c.z;
    *at++ = c.w;
  }
  geo_vertex_buffers_validate(b);
  return result;
}

internal u32
geo_vertex_buffer_builder_push_vt(Geo_Vertex_Buffer_Builder* b, v3 v, v2 t)
{
  return geo_vertex_buffer_builder_push_vtc(b, v, t, (v4){0});
}

internal u32
geo_vertex_buffer_builder_push_v(Geo_Vertex_Buffer_Builder* b, v3 v)
{
  return geo_vertex_buffer_builder_push_vtc(b, v, (v2){0}, (v4){0});
}


// Index Buffer

internal u32
geo_index_buffer_builder_push(Geo_Index_Buffer_Builder* b, u32 i)
{
  assert(b->len < b->cap);
  u32 result = b->len++;
  b->values[result] = i;
  return result;
}

internal void
geo_index_buffer_builder_push_tri(Geo_Index_Buffer_Builder* b, u32 i0, u32 i1, u32 i2)
{
  geo_index_buffer_builder_push(b, i0);
  geo_index_buffer_builder_push(b, i1);
  geo_index_buffer_builder_push(b, i2);
}

internal void
geo_index_buffer_builder_push_quad(Geo_Index_Buffer_Builder* b, u32 i0, u32 i1, u32 i2, u32 i3)
{
  geo_index_buffer_builder_push_tri(b, i0, i1, i2);
  geo_index_buffer_builder_push_tri(b, i0, i2, i3);
}


// Quad Buffer

internal void
geo_quad_buffer_builder_push_vtc(Geo_Quad_Buffer_Builder* b, v3 p0, v3 p1, v3 p2, v3 p3, v2 t0, v2 t1, v2 t2, v2 t3, v4 c)
{
  u32 i0 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p0, t0, c);
  u32 i1 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p1, t1, c);
  u32 i2 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p2, t2, c);
  u32 i3 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p3, t3, c);
  
  geo_index_buffer_builder_push_quad(&b->buffer_index, i0, i1, i2, i3);
}

internal void
geo_quad_buffer_builder_push_vt(Geo_Quad_Buffer_Builder* b, v3 p0, v3 p1, v3 p2, v3 p3, v2 t0, v2 t1, v2 t2, v2 t3)
{
  u32 i0 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p0, t0, (v4){});
  u32 i1 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p1, t1, (v4){});
  u32 i2 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p2, t2, (v4){});
  u32 i3 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p3, t3, (v4){});
  
  geo_index_buffer_builder_push_quad(&b->buffer_index, i0, i1, i2, i3);
}

internal void
geo_quad_buffer_builder_push_v(Geo_Quad_Buffer_Builder* b, v3 p0, v3 p1, v3 p2, v3 p3)
{
  u32 i0 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p0, (v2){},  (v4){});
  u32 i1 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p1, (v2){}, (v4){});
  u32 i2 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p2, (v2){}, (v4){});
  u32 i3 = geo_vertex_buffer_builder_push_vtc(&b->buffer_vertex, p3, (v2){}, (v4){});
  
  geo_index_buffer_builder_push_quad(&b->buffer_index, i0, i1, i2, i3);
}

internal Geo_Quad_Buffer
geo_quad_buffer_builder_get_static_buffer(Geo_Quad_Buffer_Builder* b)
{
  Geo_Quad_Buffer result = {};
  result.buffer_vertex = b->buffer_vertex.buffer;
  result.buffer_index = b->buffer_index.buffer;
  return result;
}

#endif //LUMENARIUM_GEOMETRY_H
