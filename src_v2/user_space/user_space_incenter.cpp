
internal App_Init_Desc
incenter_get_init_desc()
{
  App_Init_Desc result = {};
  result.assembly_cap = 4;
  return result;
}

internal void
incenter_init(App_State* state)
{
  // create a fake sculpture
  Assembly_Handle ah = assembly_add(&state->assemblies, lit_str("test"), 3000, 100);
  
  r32 scale = 1;
  
  // strips
  for (int strip_x = 0; strip_x < 10; strip_x++)
  {
    for (int strip_y = 0; strip_y < 10; strip_y++)
    {
      if (strip_x == 5 && strip_y == 7)
      {
        int x= 5;
      }
      Assembly_Strip* strip = assembly_add_strip(&state->assemblies, ah, 30);
      
      // leds go up
      for (int led_z = 0; led_z < 30; led_z++)
      {
        v4 pos = { strip_x * scale, strip_y * scale, led_z * scale, 1 };
        assembly_add_led(&state->assemblies, ah, strip, pos);
      }
    }
  }
}

internal void
incenter_frame_prepare(App_State* state)
{
  
}

internal void
incenter_frame(App_State* state)
{
  
}

internal void
incenter_cleanup(App_State* state)
{
  
}