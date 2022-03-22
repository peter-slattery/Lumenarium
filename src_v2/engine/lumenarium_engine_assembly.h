/* date = March 22nd 2022 6:40 pm */

#ifndef LUMENARIUM_ENGINE_ASSEMBLY_H
#define LUMENARIUM_ENGINE_ASSEMBLY_H

struct Assembly_Handle
{
  u32 value;
};

union Assembly_Pixel
{
  struct {
    u8 r;
    u8 g;
    u8 b;
  };
  u8 channels[3];
};

struct Assembly_Pixel_Buffer
{
  u32 cap;
  u32 len;
  Assembly_Pixel* pixels;
  v4*             positions;
};

struct Assembly_Strip
{
  u32  pixels_cap;
  u32* pixels;
};

struct Assembly_Strip_Array
{
  u32 cap;
  Assembly_Strip* strips;
};

struct Assembly_Array
{
  u32 cap;
  u32 len;
  String*                names;
  Assembly_Pixel_Buffer* pixel_buffers;
  Assembly_Strip_Array*  strip_arrays;
  
  Allocator* allocator;
};

Assembly_Handle assembly_add(Assembly_Array* a, String name, u64 pixels_cap, u64 strips_cap);
void            assembly_rem(Assembly_Array* a, Assembly_Handle h);
Assembly_Strip* assembly_add_strip(Assembly_Array* a, Assembly_Handle h);
void            assembly_add_led(Assembly_Array* a, Assembly_Handle h, Assembly_Strip* strip, v4 position);


#endif //LUMENARIUM_ENGINE_ASSEMBLY_H
