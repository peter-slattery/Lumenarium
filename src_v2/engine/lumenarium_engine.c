
internal void
en_init(App_State* state, App_Init_Desc desc)
{
  lumenarium_env_validate();
  
  state->assemblies = assembly_array_create(permanent, desc.assembly_cap);
  
  Output* o = &state->output;
  register_output_method(o, OutputData_Invalid, 0, 0, 0);
  register_output_method(o, OutputData_NetworkSACN, output_network_sacn_update, output_network_sacn_build, output_network_sacn_init());
  register_output_method(o, OutputData_ComUART, 0, output_network_sacn_build, output_com_uart_init());
  
  lumenarium_env_validate();
}

internal void
en_frame_prepare(App_State* state)
{
  lumenarium_env_validate(); 
}

global r32 tt = 0;

r32
curve_ease_in_out(r32 t)
{
  r32 tc = clamp(0, t, 1);
  r32 theta = (tc * r32_pi) - (r32_pi / 2.0f);
  r32 s = sinf(theta);
  r32 result = 0.5f + (s / 2.0f);
  return result;
}

#define incenter_pos_to_unit(p) (v4){ ((p.x / 6.0f) + 0.5f), ((p.y / 6.0f) + 0.5f), ((p.z / 6.0f) + 0.5f), 1.0f }

void 
test_pattern(Assembly_Pixel_Buffer pixels)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = pixels.positions[j];
    pixels.pixels[j].r = (u8)(((sinf((5 * tt) + (p.x * 10)) + 1) * 0.5f) * 255);
    pixels.pixels[j].b = (u8)(((sinf((5 * tt) + (p.z * 10)) + 1) * 0.5f) * 255);
  }
}

void
pattern_debug(Assembly_Pixel_Buffer pixels)
{
  r32 scale = 6;
  r32 offset = 0;

  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = pixels.positions[j];
    v4 pp = incenter_pos_to_unit(p);
    pixels.pixels[j].r = pp.x * 255;
    pixels.pixels[j].g = pp.y * 255;
    pixels.pixels[j].b = pp.z * 255;
  }
}

void
grow_pattern_sphere_function(Assembly_Pixel_Buffer pixels, v4 center, r32 radius, r32 falloff)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = incenter_pos_to_unit(pixels.positions[j]);
    r32 d0 = HMM_LengthVec4(HMM_SubtractVec4(p, center));
    r32 d1 = falloff - fabsf(d0 - radius);
    r32 b = d1 / falloff;

    v3 color = {
      .x = 0.5f + 0.5f * sinf(p.x * r32_tau * 4.313f + tt * 0.53f),
      .y = 0.5f + 0.5f * cosf(0.2314f + p.y * r32_tau * 3.915f + tt * 0.5f),
      .z = 0.2f + 0.8f * p.z,
    };
    v3 color_b = HMM_MultiplyVec3f(color, b);

    pixels.pixels[j].r = color_b.x * 255;
    pixels.pixels[j].g = color_b.y * 255;
    pixels.pixels[j].b = color_b.z * 255;
  }
}
void 
grow_pattern(Assembly_Pixel_Buffer pixels)
{
  v4 center = (v4){};
  r32 radius = 0;
  r32 falloff = 0;
  //phase 1 - light up to center
  if (tt < 8)
  {
    r32 height = -0.2f + curve_ease_in_out(tt / 6) * 0.5f;
    center = (v4){ 0.5f, 0.2f + height, 0.5f, 1 };
    radius = 0.1f;
    falloff = 0.2f;
  }
  else if (tt >= 8)
  {
    r32 t = (tt - 8) / 5;
    center = (v4){ 0.5f, 0.5f, 0.5f, 1 };
    radius = 0.1f + curve_ease_in_out(t) * 0.27f;
    falloff = 0.2 - (curve_ease_in_out(t) * 0.1f);
  }

  grow_pattern_sphere_function(pixels, center, radius, falloff);
}

internal void
en_frame(App_State* state)
{
  lumenarium_env_validate();

  scratch_get(scratch);
  Assembly_Array assemblies = state->assemblies;
  
  ///////////////////////////////////////
  // Temp Pattern Simulation
  
  tt += 1.0f / 60.0f;
  if (tt > 50) tt = 0;
  
  r32 hrange = 1;
  r32 range = hrange * 2;
  for (u32 i = 0; i < assemblies.len; i++)
  {
    Assembly_Pixel_Buffer pixels = assemblies.pixel_buffers[i];
    grow_pattern(pixels);
  }
  
  ///////////////////////////////////////
  // Output Data
  Output_Methods methods = state->output.methods;

  // update each method that has an update method
  for (u32 i = 0; i < OutputData_Count; i++)
  {
    if (methods.update_procs[i] == 0) continue;
    methods.update_procs[i](methods.method_data[i]);
  }

  Output_Data_Queue output_queue = {};
  output_queue.a = scratch.a;
  
  // build output data buffers
  for (u32 i = 0; i < assemblies.len; i++)
  {
    Assembly_Strip_Array strips = assemblies.strip_arrays[i];
    Assembly_Pixel_Buffer* pixels = assemblies.pixel_buffers + i;
    for (u32 j = 0; j < strips.len; j++)
    {
      Assembly_Strip* strip = strips.strips + j;
      Build_Output_Data_Buffer* method_proc = methods.procs[strip->output_kind];
      u8* method_data = methods.method_data[strip->output_kind];
      if (method_proc == 0) continue;
      method_proc(state, i, pixels, strip, method_data, &output_queue);
    }
  }
  
  // output the buffers
  // TODO(PS): we could sort these first if switchig between output 
  // types is time consuming
  
  Sacn* sacn = (Sacn*)state->output.methods.method_data[OutputData_NetworkSACN];
  for (Output_Data* at = output_queue.first; at != 0; at = at->next)
  {
    // NOTE(PS): we can overload each switch case as more methods come in
    // ie. OutputData_NetworkSACN and OutputData_NetworkArtNet could use
    // the same case since at this point they just need to push data out
    // over a socket
    switch (at->kind)
    {
      case OutputData_NetworkSACN:
      {
        os_socket_send_to(
          sacn->socket,
          at->network.v4_addr,
          at->network.port,
          at->data,
          0
        );
      } break;
      
      case OutputData_ComUART:
      {
        // TODO(PS): platform com io
      } break;
      
      invalid_code_path;
    }
  }

  scratch_release(scratch);
  lumenarium_env_validate();
}

internal void
en_cleanup(App_State* state)
{
  lumenarium_env_validate();
}

