
Assembly_Array
assembly_array_create(Allocator* allocator, u32 cap)
{
  Assembly_Array result = {};
  result.cap = cap;
  result.names = allocator_alloc_array(allocator, String, cap);
  result.pixel_buffers = allocator_alloc_array(allocator, Assembly_Pixel_Buffer, cap);
  result.strip_arrays = allocator_alloc_array(allocator, Assembly_Strip_Array, cap);
  result.allocator = allocator;
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
  pixel_buffer->cap = pixels_cap;
  pixel_buffer->pixels = allocator_alloc_array(a->allocator, Assembly_Pixel, pixels_cap);
  pixel_buffer->positions = allocator_alloc_array(a->allocator, v4, pixels_cap);
  
  Assembly_Strip_Array* strip_array = a->strip_arrays + index;
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
  pixel_buffer->pixels[pixel_index] = {};
  pixel_buffer->positions[pixel_index] = position;
  
  assert(strip->pixels_len < strip->pixels_cap);
  strip->pixels[strip->pixels_len++] = pixel_index;
}
