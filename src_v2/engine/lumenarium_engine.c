
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

#include "../user_space/incenter_patterns.c"

internal void
en_frame(App_State* state)
{
  lumenarium_env_validate();

  scratch_get(scratch);
  Assembly_Array assemblies = state->assemblies;
  
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

