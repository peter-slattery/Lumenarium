
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
  Assembly_Handle ah = assembly_add(&state->assemblies, lit_str("test"), 7043, city_count + 1);
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
    (v3){0, INCENTER_FEET(-4.5f), 0}, 
    123
  );
  
  r32 radius = INCENTER_FEET(10);
  
  Random_Series rand = random_series_create(hash_djb2_cstr_to_u32("slfsaassdjf"));
  u32 i = 0;
  while (i < city_count)
  //for (u32 i = 0; i < 40; i++)
  {
    Incenter_City_Desc city = city_descs[i];

#if 0
    // convert lat/lon (degrees) to theta/phi (radians)
    // lat is in the range -90deg to 90deg
    // lon is in the range -180deg  to 180deg
    // we want both in the range 0 to 2
    r32 theta = ((city.lat + 90) / 180.0f) * r32_pi;
    r32 phi   = ((city.lon + 180)  / 360.0f) * r32_tau;

    printf("%s - \t\t%f %f -> \t\t%f %f\n",
      city_strings[city.id],
      city.lat, city.lon,
      theta, phi
    );
#else
    r32 theta = random_series_next_unilateral(&rand) * r32_tau;
    r32 phi   = random_series_next_unilateral(&rand) * r32_tau;
#endif

    // spherical to cartesian conversion
    v3 end_p = {
      radius * sinf(phi) * cosf(theta),
      radius * sinf(phi) * sinf(theta),
      radius * cosf(phi)
    };

    r32 down = HMM_DotVec3(HMM_NormalizeVec3(end_p), (v3){0, -1, 0});
    //if (down > 0.7f || down < -0.9f) continue;

    Assembly_Strip* strip = assembly_add_strip(&state->assemblies, ah, 123);
    strip->output_kind = OutputData_NetworkSACN;
    strip->sacn_universe = city.sacn_universe;
    
    assembly_strip_create_leds(&state->assemblies, ah, strip, start_p, end_p, 123);
    i++;
  }
  
  r32 rad = 0.05f;
  sculpture_updated(state, 5, rad);
  scratch_release(scratch);
}

internal void
incenter_frame_prepare(App_State* state)
{
  
}

global u32 pattern_sel = 2;

internal void
incenter_frame(App_State* state)
{
  Assembly_Array assemblies = state->assemblies;

  if (input_key_is_down(state->input_state, KeyCode_0)) { tt = 0; pattern_sel = 0; }
  if (input_key_is_down(state->input_state, KeyCode_1)) { tt = 0; pattern_sel = 1; }
  if (input_key_is_down(state->input_state, KeyCode_2)) { tt = 0; pattern_sel = 2; }
  if (input_key_is_down(state->input_state, KeyCode_3)) { tt = 0; pattern_sel = 3; }
  if (input_key_is_down(state->input_state, KeyCode_4)) { tt = 0; pattern_sel = 4; }
  if (input_key_is_down(state->input_state, KeyCode_5)) { tt = 0; pattern_sel = 5; }

  ///////////////////////////////////////
  // Temp Pattern Simulation
  
  tt += 1.0f / 60.0f;
  if (tt > 50) tt = 0;
  
  switch (pattern_sel) {
    case 0:  { pattern_color(assemblies.pixel_buffers[0], assemblies.strip_arrays[0], 255, 255, 255); } break;
    case 1:  { pattern_demo_1(assemblies.pixel_buffers[0], assemblies.strip_arrays[0]); } break;
    case 2:  { pattern_demo_2(assemblies.pixel_buffers[0], assemblies.strip_arrays[0]); } break;
    case 3:  { pattern_demo_3(assemblies.pixel_buffers[0], assemblies.strip_arrays[0]); } break;
    default: { pattern_demo (assemblies.pixel_buffers[0], assemblies.strip_arrays[0]); } break;
  }

#if 0
  r32 hrange = 1;
  r32 range = hrange * 2;
  for (u32 i = 0; i < assemblies.len; i++)
  {
    Assembly_Pixel_Buffer pixels = assemblies.pixel_buffers[i];
    //grow_pattern(pixels);
    //pattern_color(pixels, 0, 0, 0);
    pattern_demo(pixels, assemblies.strip_arrays[i]);
    // pattern_test_data_scene_hombre(
    //   pixels, 
    //   assemblies.strip_arrays[i],
    //   (Assembly_Pixel){ .r = 32, .g = 128, .b = 255 },
    //   (Assembly_Pixel){ .r = 255, .g = 200, .b = 32 },
    //   (Assembly_Pixel){ .r = 255, .g = 255, .b = 255 },
    //   (Assembly_Pixel){ .r = 64, .g = 0, .b = 0 }
    // );
  } 
#endif
}

internal void
incenter_cleanup(App_State* state)
{
  
}