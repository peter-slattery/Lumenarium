
internal void
en_init(App_State* state, App_Init_Desc desc)
{
  state->assemblies = assembly_array_create(permanent, desc.assembly_cap);
  
  Output* o = &state->output;
  register_output_method(o, OutputData_NetworkSACN, output_network_sacn_build, output_network_sacn_init());
  register_output_method(o, OutputData_ComUART, output_network_sacn_build, output_com_uart_init());
}

internal void
en_frame_prepare(App_State* state)
{
  
}

internal void
en_frame(App_State* state)
{
#if 0
  ///////////////////////////////////////
  // Output Data
  
  Output_Data_Queue output_queue = {};
  output_queue.a = scratch;
  
  // build output data buffers
  Output_Methods methods;
  Assembly_Array assemblies = state->assemblies;
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
#endif
}

internal void
en_cleanup(App_State* state)
{
  
}

