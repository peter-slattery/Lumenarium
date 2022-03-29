/* date = March 28th 2022 10:52 pm */

#ifndef LUMENARIUM_UI_H
#define LUMENARIUM_UI_H

struct Font_Glyph
{
  u32 code_point;
  v2 uv_min;
  v2 uv_max;
};

struct Font_Glyph_Table
{
  Font_Glyph* values;
  u32 cap;
  u32 len;
};

struct Font_Bitmap
{
  u8* pixels;
  u32 width;
  u32 height;
  u32 stride;
  
  Platform_Texture texture;
};

struct Font
{
  Font_Glyph_Table glyphs;
  Font_Bitmap bitmap;
};

#endif //LUMENARIUM_UI_H
