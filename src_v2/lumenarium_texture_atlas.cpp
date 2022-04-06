/* date = March 31st 2022 8:30 pm */

#ifndef LUMENARIUM_TEXTURE_ATLAS_CPP
#define LUMENARIUM_TEXTURE_ATLAS_CPP

struct Texture_Atlas_Sprite
{
  u16 min_x;
  u16 min_y;
  u16 max_x;
  u16 max_y;
};

struct Texture_Atlas
{
  u8* pixels;
  u16 width;
  u16 height;
  
  u16 next_x;
  u16 next_y;
  u16 y_used;
  
  u32* ids;
  Texture_Atlas_Sprite* sprites;
  u32 cap;
  u32 used;
};

internal Texture_Atlas
texture_atlas_create(u32 width, u32 height, u32 cap, Allocator* allocator)
{
  Texture_Atlas result = {};
  result.pixels = allocator_alloc_array(allocator, u8, width * height * 4);
  result.width = (u16)width;
  result.height = (u16)height;
  for (u32 i = 0; i < width * height; i++) {
    u8* base = result.pixels + (i * 4);
    *(u32*)base = 0xFFFFFFFF;
  }
  
  result.ids = allocator_alloc_array(allocator, u32, cap);
  result.sprites = allocator_alloc_array(allocator, Texture_Atlas_Sprite, cap);
  result.cap = cap;
  hash_table_init(result.ids, cap);
  return result;
}

internal void
texture_atlas_register(Texture_Atlas* ta, u8* pixels, u32 width, u32 height, u32 id)
{
  u16 min_x = ta->next_x;
  u16 min_y = ta->next_y;
  u16 max_x = min_x + (u16)width;
  u16 max_y = min_y + (u16)height;
  
  // TODO(PS): if the sprite won't fit in this row, then we need to shift it to 
  // the next one
  
  // copy the data
  for (u16 y = 0; y < height; y++)
  {
    u16 src_row = (y * (u16)width) * 4;
    u16 dst_row = (((y + min_y) * ta->width) + min_x) * 4;
    for (u16 x = 0; x < width; x++)
    {
      ta->pixels[dst_row++] = pixels[src_row++];
      ta->pixels[dst_row++] = pixels[src_row++];
      ta->pixels[dst_row++] = pixels[src_row++];
      ta->pixels[dst_row++] = pixels[src_row++];
    }
  }
  
  // register a new slot
  u32 index = hash_table_register(ta->ids, ta->cap, id);
  
  Texture_Atlas_Sprite* sprite = ta->sprites + index;
  sprite->min_x = min_x;
  sprite->min_y = min_y;
  sprite->max_x = max_x;
  sprite->max_y = max_y;
  
  // Prepare for next registration
  if (max_y > ta->y_used) 
  {
    ta->y_used = max_y;
  }
  
  ta->next_x = max_x + 1;
  if (ta->next_x > ta->width)
  {
    ta->next_x = 0;
    ta->next_y = ta->y_used;
  }
}

internal Texture_Atlas_Sprite
texture_atlas_sprite_get(Texture_Atlas* ta, u32 id)
{
  Texture_Atlas_Sprite result = {};
  u32 index = hash_table_find(ta->ids, ta->cap, id);
  if (index == ta->cap) return result;
  result = ta->sprites[index];
  return result;
}

internal v4
texture_atlas_sprite_get_uvs(Texture_Atlas* ta, u32 id)
{
  Texture_Atlas_Sprite sprite = texture_atlas_sprite_get(ta, id);
  v4 result = {};
  
  // uv min
  result.x = (r32)sprite.min_x / (r32)ta->width;
  result.y = (r32)sprite.min_y / (r32)ta->height;
  
  // uv max
  result.z = (r32)sprite.max_x / (r32)ta->width;
  result.w = (r32)sprite.max_y / (r32)ta->height;
  
  // inset
  v2 half_texel = v2{1.0f / ta->width, 1.0f / ta->height};
  result.xy += half_texel;
  result.zw -= half_texel;
  
  return result;
}
#endif //LUMENARIUM_TEXTURE_ATLAS_CPP
