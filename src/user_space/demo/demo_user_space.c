#define DEMO_STRIP_LEDS_CAP 123

internal void
demo_push_strip(Assembly_Array* aa, Assembly_Handle ah, u32 universe, v3 start, v3 end)
{
  Assembly_Strip* strip = assembly_add_strip(aa, ah, DEMO_STRIP_LEDS_CAP);
  strip->output_kind = OutputData_NetworkSACN;
  strip->sacn_universe = universe;
  assembly_strip_append_leds(aa, ah, strip, start, end, DEMO_STRIP_LEDS_CAP);
}

internal void
demo_init(App_State* state)
{
  Assembly_Array* aa = &state->assemblies;
  u32 strips_dim = 9;
  u32 strips_cap = strips_dim * strips_dim;
  Assembly_Handle ah = assembly_add(aa, lit_str("Demo"), strips_cap * DEMO_STRIP_LEDS_CAP, strips_cap);

  scratch_get(scratch);
  Allocator* s = scratch.a;

  r32 r = 0.5f;
  s32 start = -(strips_dim / 2);
  s32 end = strips_dim / 2;
  r32 z_min = -((r * strips_dim) / 2.0f);
  r32 z_max =  ((r * strips_dim) / 2.0f);
  for (s32 y = start; y < end; y++)
  {
    for (s32 x = start; x < end; x++)
    {
      demo_push_strip(aa, ah, 1, (v3){x * r, y * r, z_min}, (v3){ x * r, y * r, z_max});
    }
  }

  sculpture_updated(state, 5, 0.1f);
}

internal void
demo_frame_prepare(App_State* state)
{

}

#if defined(PLATFORM_SUPPORTS_EDITOR)
internal void
demo_sculpture_visualizer_ui(App_State* state, Editor* ed)
{
}
#endif

global Color_Ramp aurora_ramp = {
  .anchors = {
    [0] = { .pct = 0,    .color = { 0, 0, 0 } },
    [1] = { .pct = .4f,  .color = { 0, 0, 0 } },
    [2] = { .pct = .55f,  .color = { 176.f / 255.f, 65.f / 255.f, 36.f / 255.f } },
    [3] = { .pct = .7f,  .color = { 237.f / 255.f, 201.f / 255.f, 138.f / 255.f } },
    [4] = { .pct = .80f, .color = { 49.f / 255.f, 156.f / 255.f, 255.f / 255.f } },
    [5] = { .pct = 1.0f, .color = { 49.f / 255.f, 156.f / 255.f, 255.f / 255.f } },
  },
  .anchors_count = 5,
};

void
pattern_aurora(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, r32 scene_time)
{
  for (u32 i = 0; i < pixels.len; i++)
  {
    v3 p = pixels.positions[i].xyz;
    v3 p_offset = HMM_AddVec3(p, (v3){ 213.145f, 99.321f, 71.3f });
    v3 p_scaled = HMM_MultiplyVec3f(p_offset, 0.25f);
    r32 v = pm_fmb_3d(p_scaled, scene_time);
    r32 vv = pm_smoothstep_r32(v);
    v3 color = color_ramp_eval(aurora_ramp, vv);
    pixels.pixels[i] = color_v3_to_assembly_pixel(color);
  }
}

internal void
demo_frame(App_State* state)
{
  local_persist r64 t = 0;
  t += state->target_seconds_per_frame;

  Assembly_Array assemblies = state->assemblies;
  for (u32 assembly_i = 0; assembly_i < assemblies.len; assembly_i++)
  {
    Assembly_Strip_Array* strips = assemblies.strip_arrays + assembly_i;
    Assembly_Pixel_Buffer* pixels = assemblies.pixel_buffers + assembly_i;
    pattern_aurora(*pixels, *strips, (r32)t);
  } 
}

internal void
demo_cleanup(App_State* state)
{

}

internal App_Init_Desc
user_space_get_init_desc()
{
  return (App_Init_Desc){
    .assembly_cap = 1,
    .init = demo_init,
    .frame_prepare = demo_frame_prepare,
#if defined(PLATFORM_SUPPORTS_EDITOR)
    .sculpture_visualizer_ui = demo_sculpture_visualizer_ui,
#endif
    .frame = demo_frame,
    .cleanup = demo_cleanup,
  };
}