
internal App_Init_Desc
incenter_get_init_desc()
{
  App_Init_Desc result = {};
  result.assembly_cap = 4;
  return result;
}

#define INCENTER_METER 1.0f
#define INCENTER_FOOT 0.3048f
#define INCENTER_METERS(count) (count) * INCENTER_METER
#define INCENTER_FEET(count) (count) * INCENTER_FOOT
#define INCENTER_PER_METER(count) INCENTER_METER / (r32)(count)

internal void
incenter_init(App_State* state)
{
  // create a fake sculpture
  Assembly_Handle ah = assembly_add(&state->assemblies, lit_str("test"), 5043, 41);
  //Assembly_Handle ah = assembly_add(&state->assemblies, lit_str("test"), 20000, 150);
  
  scratch_get(scratch);
  Allocator* s = scratch.a;
  
  v3 start_p = (v3){0, 0, 0};
  
  Assembly_Strip* vertical_strip = assembly_add_strip(&state->assemblies, ah, 123);
  assembly_strip_create_leds(
    &state->assemblies, 
    ah,
    vertical_strip, 
    start_p,
    (v3){0, INCENTER_FEET(-6.5f), 0}, 
    123
  );
  
  r32 radius = INCENTER_FEET(10);
  
  Random_Series rand = random_series_create(hash_djb2_cstr_to_u32("slfalksdjf"));
  for (u32 i = 0; i < 40; i++)
  {
    Assembly_Strip* strip = assembly_add_strip(&state->assemblies, ah, 123);
    strip->output_kind = OutputData_NetworkSACN;
    strip->sacn_universe = i;
    
    r32 theta = random_series_next_unilateral(&rand) * r32_tau;
    r32 phi   = random_series_next_unilateral(&rand) * r32_tau;
    
    // spherical to cartesian conversion
    v3 end_p = {
      radius * sinf(phi) * cosf(theta),
      radius * sinf(phi) * sinf(theta),
      radius * cosf(phi)
    };
    assembly_strip_create_leds(&state->assemblies, ah, strip, start_p, end_p, 123);
  }
  
  r32 rad = 0.05f;
  ed_sculpture_updated(state, 5, rad);
  scratch_release(scratch);
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