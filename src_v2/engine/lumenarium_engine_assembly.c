
Assembly_Array
assembly_array_create(Allocator* allocator, u32 cap)
{
  Assembly_Array result = {
    .cap = cap,
    .names = allocator_alloc_array(allocator, String, cap),
    .pixel_buffers = allocator_alloc_array(allocator, Assembly_Pixel_Buffer, cap),
    .strip_arrays = allocator_alloc_array(allocator, Assembly_Strip_Array, cap),
    .allocator = allocator,
  };
  return result;
}

bool
assembly_handle_is_valid(Assembly_Handle h)
{
  return ((h.value & ASSEMBLY_HANDLE_VALID_BIT) != 0);
}

u32
assembly_handle_to_index(Assembly_Handle h)
{
  assert(assembly_handle_is_valid(h));
  u32 index = (h.value & ASSEMBLY_HANDLE_INDEX_MASK); // mask off high bit
  return index;
}

Assembly_Handle
assembly_add(Assembly_Array* a, String name, u32 pixels_cap, u32 strips_cap)
{
  Assembly_Handle result = {};
  if (a->len < a->cap)
  {
    result.value = a->len++;
    result.value |= ASSEMBLY_HANDLE_VALID_BIT; // high bit being set means its valid
  }
  else
  {
    // TODO(PS): find empty index
    // we can use the name being zero as a signal that the slot is empty
  }
  
  // NOTE(PS): not actually a bug, just go increase App_Init_Desc::assemblies_cap
  // b/c you ran out of room
  assert(assembly_handle_is_valid(result));
  
  u32 index = assembly_handle_to_index(result);
  
  a->names[index] = name;
  
  Assembly_Pixel_Buffer* pixel_buffer = a->pixel_buffers + index;
  zero_struct(*pixel_buffer);
  pixel_buffer->cap = pixels_cap;
  pixel_buffer->pixels = allocator_alloc_array(a->allocator, Assembly_Pixel, pixels_cap);
  pixel_buffer->positions = allocator_alloc_array(a->allocator, v4, pixels_cap);
  
  Assembly_Strip_Array* strip_array = a->strip_arrays + index;
  zero_struct(*strip_array);
  strip_array->cap = strips_cap;
  strip_array->strips = allocator_alloc_array(a->allocator, Assembly_Strip, strips_cap);
  
  return result;
}

void
assembly_rem(Assembly_Array* a, Assembly_Handle h)
{
}

Assembly_Strip*
assembly_add_strip(Assembly_Array* a, Assembly_Handle h, u32 pixels_cap)
{
  u32 index = assembly_handle_to_index(h);
  
  Assembly_Strip_Array* strip_array = a->strip_arrays + index;
  assert(strip_array->len < strip_array->cap);
  
  Assembly_Strip* result = strip_array->strips + strip_array->len++;
  result->pixels_cap = pixels_cap;
  result->pixels_len = 0;
  result->pixels = allocator_alloc_array(a->allocator, u32, pixels_cap);
  
  return result;
}

void
assembly_add_led(
    Assembly_Array* a, 
  Assembly_Handle h, 
  Assembly_Strip* strip, 
  v4 position
){
  u32 index = assembly_handle_to_index(h);
  
  Assembly_Pixel_Buffer* pixel_buffer = a->pixel_buffers + index;
  assert(pixel_buffer->len < pixel_buffer->cap);
  
  u32 pixel_index = pixel_buffer->len++;
  pixel_buffer->pixels[pixel_index] = (Assembly_Pixel){};
  pixel_buffer->positions[pixel_index] = position;
  
  assert(strip->pixels_len < strip->pixels_cap);
  strip->pixels[strip->pixels_len++] = pixel_index;
}

void
assembly_strip_append_leds(
  Assembly_Array* a, 
  Assembly_Handle h, 
  Assembly_Strip* strip, 
  v3 start, v3 end, 
  u32 led_count 
){
  v3 delta_total = HMM_SubtractVec3(end, start);
  v3 delta_step = HMM_MultiplyVec3f(delta_total, 1.0f / (r32)led_count);
  
  for (u32 i = 0; i < led_count; i++)
  {
    v4 pos = {0,0,0,1};
    v3 offset = HMM_MultiplyVec3f(delta_step, (r32)i);
    pos.XYZ = HMM_AddVec3(start, offset);
    assembly_add_led(a, h, strip, pos);
  }
}