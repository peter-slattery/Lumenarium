/* date = March 22nd 2022 6:40 pm */

#ifndef LUMENARIUM_ENGINE_ASSEMBLY_H
#define LUMENARIUM_ENGINE_ASSEMBLY_H

// Assembly_Handle is valid for any index, including zero. However, 
// valid values must have the high bit set. This way, the handle declared
// via Assembly_Handle my_handle = {}; is invalid, while still allowing
// index zero to be used in the array.
// 
// If memory corruption becomes an issue we can make this a bigger bit
// field that we check since Lumenarium doesn't ever really expect to have
// more than 128 sculptures in a scene - but who knows, maybe someday O.o?
#define ASSEMBLY_HANDLE_VALID_BIT (1 << 31)
#define ASSEMBLY_HANDLE_INDEX_MASK ~ASSEMBLY_HANDLE_VALID_BIT
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
  u32  pixels_len;
  // array of indices into the Assembly_Pixel_Buffer for the same assembly
  u32* pixels;
};

struct Assembly_Strip_Array
{
  u32 cap;
  u32 len;
  Assembly_Strip* strips;
};

struct Assembly_Array
{
  u32 cap;
  u32 len;
  
  // assembly names
  String*                names;
  
  // each assembly gets its own pixel buffer
  Assembly_Pixel_Buffer* pixel_buffers;
  
  // each assembly gets its own array of strips which
  // index into that assemblies pixel_buffer
  Assembly_Strip_Array*  strip_arrays;
  
  Allocator* allocator;
};

Assembly_Array  assembly_array_create(Allocator* allocator, u32 cap);
Assembly_Handle assembly_add(Assembly_Array* a, String name, u32 pixels_cap, u32 strips_cap);
void            assembly_rem(Assembly_Array* a, Assembly_Handle h);
Assembly_Strip* assembly_add_strip(Assembly_Array* a, Assembly_Handle h, u32 pixels_cap);
void            assembly_add_led(Assembly_Array* a, Assembly_Handle h, Assembly_Strip* strip, v4 position);


#endif //LUMENARIUM_ENGINE_ASSEMBLY_H
