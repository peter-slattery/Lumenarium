
internal void
en_init(App_State* state, App_Init_Desc desc)
{
  lumenarium_env_validate();
  
  state->assemblies = assembly_array_create(permanent, desc.assembly_cap);
  
  Output* o = &state->output;
  register_output_method(o, OutputData_Invalid, 0, 0);
  register_output_method(o, OutputData_NetworkSACN, output_network_sacn_build, output_network_sacn_init());
  register_output_method(o, OutputData_ComUART, output_network_sacn_build, output_com_uart_init());
  
  lumenarium_env_validate();
}

internal void
en_frame_prepare(App_State* state)
{
  lumenarium_env_validate(); 
}

global r32 tt = 0;

internal void
en_frame(App_State* state)
{
  lumenarium_env_validate();

  scratch_get(scratch);
  Assembly_Array assemblies = state->assemblies;
  
  ///////////////////////////////////////
  // Temp Pattern Simulation
  
  tt += 1.0f / 60.0f;
  
  r32 hrange = 1;
  r32 range = hrange * 2;
  for (u32 i = 0; i < assemblies.len; i++)
  {
    Assembly_Pixel_Buffer pixels = assemblies.pixel_buffers[i];
    for (u32 j = 0; j < pixels.len; j++)
    {
      v4 p = pixels.positions[j];
      pixels.pixels[j].r = (u8)(((sinf((5 * tt) + (p.x * 10)) + 1) * 0.5f) * 255);
      pixels.pixels[j].b = (u8)(((sinf((5 * tt) + (p.z * 10)) + 1) * 0.5f) * 255);
    }
  }
  
  ///////////////////////////////////////
  // Output Data
  
  Output_Data_Queue output_queue = {};
  output_queue.a = scratch.a;
  
  // build output data buffers
  Output_Methods methods = state->output.methods;
  for (u32 i = 0; i < assemblies.len; i++)
  {
    Assembly_Strip_Array strips = assemblies.strip_arrays[i];
    for (u32 j = 0; j < strips.len; j++)
    {
      Assembly_Strip* strip = strips.strips + j;
      Build_Output_Data_Buffer* method_proc = methods.procs[strip->output_kind];
      u8* method_data = methods.method_data[strip->output_kind];
      if (method_proc == 0) continue;
      method_proc(state, i, strip, method_data, &output_queue);
    }
  }
  
  // output the buffers
  // TODO(PS): we could sort these first if switchig between output 
  // types is time consuming
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
        // TODO(PS): platform network io
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

