/* date = March 31st 2022 8:30 pm */

#ifndef LUMENARIUM_TEXTURE_ATLAS_CPP
#define LUMENARIUM_TEXTURE_ATLAS_CPP

struct Texture_Atlas_Sprite
{
  u16 min_x;
  u16 min_y;
  u16 max_x;
  u16 max_y;
  
  v2 draw_offset;
};

struct Texture_Atlas
{
  u8* pixels;
  u32 width;
  u32 height;
  
  u32 next_x;
  u32 next_y;
  u32 y_used;
  
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
    *(u32*)base = 0x00FFFFFF;
  }
  
  result.ids = allocator_alloc_array(allocator, u32, cap);
  result.sprites = allocator_alloc_array(allocator, Texture_Atlas_Sprite, cap);
  result.cap = cap;
  hash_table_init(result.ids, cap);
  return result;
}

enum Texture_Atlas_Registration_Flags
{
  TextureAtlasRegistration_None = 0,
  TextureAtlasRegistration_PixelFormat_RGBA = 1,
  TextureAtlasRegistration_PixelFormat_Alpha = 2,
};

internal void
texture_atlas_register(Texture_Atlas* ta, u8* pixels, u32 width, u32 height, u32 id, v2 draw_offset, u32 flags)
{
  if (ta->next_x > ta->width || (ta->next_x + width + 2) > ta->width)
  {
    ta->next_x = 0;
    ta->next_y = ta->y_used;
  }
  
  u32 min_x = ta->next_x + 1;
  u32 min_y = ta->next_y + 1;
  u32 max_x = min_x + width;
  u32 max_y = min_y + height;
  
  // copy the data
  if (has_flag(flags, TextureAtlasRegistration_PixelFormat_RGBA))
  {
    for (u32 y = 0; y < height; y++)
    {
      u32 src_row = (y * width) * 4;
      u32 dst_row = (((y + min_y) * ta->width) + min_x) * 4;
      for (u32 x = 0; x < width; x++)
      {
        ta->pixels[dst_row++] = pixels[src_row++];
        ta->pixels[dst_row++] = pixels[src_row++];
        ta->pixels[dst_row++] = pixels[src_row++];
        ta->pixels[dst_row++] = pixels[src_row++];
      }
    }
  }
  else if (has_flag(flags, TextureAtlasRegistration_PixelFormat_Alpha))
  {
    for (u32 y = 0; y < height; y++)
    {
      u32 src_row = y * width;
      u32 dst_row = (((y + min_y) * ta->width) + min_x) * 4;
      for (u32 x = 0; x < width; x++)
      {
        ta->pixels[dst_row++] = 0xFF;
        ta->pixels[dst_row++] = 0xFF;
        ta->pixels[dst_row++] = 0xFF;
        ta->pixels[dst_row++] = pixels[src_row++];
      }
    }
  }
  
  // copy nearest pixels to the border
  u32 pi_width = ta->width;
  u32 pi_stride = 4;
#define PIXEL_INDEX(x,y) ((((y) * pi_width) + (x)) * pi_stride)
#define COPY_PIXEL(db,sb) \
ta->pixels[(db) + 0] = ta->pixels[(sb) + 0]; \
ta->pixels[(db) + 1] = ta->pixels[(sb) + 1]; \
ta->pixels[(db) + 2] = ta->pixels[(sb) + 2]; \
ta->pixels[(db) + 3] = ta->pixels[(sb) + 3];
  
  for (u32 x = 0; x < width; x++)
  {
    u32 top      = PIXEL_INDEX(min_x + x, min_y - 1);
    u32 top_near = PIXEL_INDEX(min_x + x, min_y);
    u32 bot      = PIXEL_INDEX(min_x + x, max_y);
    u32 bot_near = PIXEL_INDEX(min_x + x, max_y - 1);
    COPY_PIXEL(top, top_near);
    COPY_PIXEL(bot, bot_near);
  }
  
  for (u32 y = 0; y < height + 2; y++)
  {
    u32 left       =  PIXEL_INDEX(min_x - 1, min_y + y - 1);
    u32 left_near  =  PIXEL_INDEX(min_x,     min_y + y - 1);
    u32 right      = PIXEL_INDEX(max_x,     min_y + y - 1);
    u32 right_near = PIXEL_INDEX(max_x - 1, min_y + y - 1);
    COPY_PIXEL(left, left_near);
    COPY_PIXEL(right, right_near);
  }
  
#undef PIXEL_INDEX
#undef COPY_PIXEL
  
  // register a new slot
  u32 index = hash_table_register(ta->ids, ta->cap, id);
  
  Texture_Atlas_Sprite* sprite = ta->sprites + index;
  sprite->min_x = (u16)min_x;
  sprite->min_y = (u16)min_y;
  sprite->max_x = (u16)max_x;
  sprite->max_y = (u16)max_y;
  sprite->draw_offset = draw_offset;
  
  // Prepare for next registration
  if (max_y > ta->y_used) 
  {
    ta->y_used = max_y + 2;
  }
  
  ta->next_x = max_x + 2;
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
texture_atlas_sprite_get_uvs(Texture_Atlas* ta, Texture_Atlas_Sprite sprite)
{
  v4 result = {};
  
  // uv min
  result.x = (r32)sprite.min_x / (r32)ta->width;
  result.y = (r32)sprite.min_y / (r32)ta->height;
  
  // uv max
  result.z = (r32)sprite.max_x / (r32)ta->width;
  result.w = (r32)sprite.max_y / (r32)ta->height;
  
  return result;
}

internal v4
texture_atlas_sprite_get_uvs(Texture_Atlas* ta, u32 id)
{
  Texture_Atlas_Sprite sprite = texture_atlas_sprite_get(ta, id);
  return texture_atlas_sprite_get_uvs(ta, sprite);
}
#endif //LUMENARIUM_TEXTURE_ATLAS_CPP
