
#define INCENTER_METER 1.0f
#define INCENTER_FOOT 0.3048f
#define INCENTER_METERS(count) (count) * INCENTER_METER
#define INCENTER_FEET(count) (count) * INCENTER_FOOT
#define INCENTER_PER_METER(count) INCENTER_METER / (r32)(count)

internal v4
incenter_latlng_to_cartesian(r32 lat, r32 lng, r32 radius)
{
  r32 theta = (lat / 180.0f) * r32_pi;
  r32 phi   = (lng / 180.0f) * r32_pi;

  // spherical to cartesian conversion
  v4 result = {
    radius * sinf(phi) * cosf(theta),
    radius * sinf(phi) * sinf(theta),
    radius * cosf(phi),
    1
  };

  return result;
}

#include "../user_space/incenter_patterns.c"
#include "../user_space/incenter_secondary_patterns.c"
#include "incenter_scenes.h"

////////////////////////////////////////////////
// INCENTER SCENES

internal void
incenter_scenes_init(Incenter_State* ins, u32 cap, Allocator* a)
{
  incenter_scene_descs_init();
  ins->scenes = incenter_scene_descs;
  ins->scenes_cap = Incenter_Scene_Count;
  ins->scene_at = Incenter_Scene_WelcomeHome;
}

internal void
incenter_scene_go_to(Incenter_State* ins, u32 index)
{
  ins->transition_time = 0;
  ins->scene_next = index % ins->scenes_cap;
  ins->scene_mode = Incenter_SceneMode_TransitioningOut;
}

internal void
incenter_scene_go_to_next(Incenter_State* ins)
{
  incenter_scene_go_to(ins, ins->scene_at + 1);
}

internal void
incenter_scene_go_to_prev(Incenter_State* ins)
{
  incenter_scene_go_to(ins, ins->scene_at - 1);
}

internal void
incenter_scene_set_mode(Incenter_State* ins, Incenter_Scene_Mode mode)
{
  ins->scene_mode = mode;
}

internal void
incenter_scene_render(App_State* state, Incenter_State* ins)
{
  assert(ins->scene_mode != Incenter_SceneMode_Count);
  Incenter_Pattern* pattern = 0;
  if (ins->scene_mode > Incenter_SceneMode_Count)
  {
    // Update the transition if necessary
    if (ins->transition_time >= INCENTER_TRANSITION_DURATION) {
      if (ins->scene_mode == Incenter_SceneMode_TransitioningOut) {
        ins->transition_time = 0;
        ins->scene_mode = Incenter_SceneMode_TransitioningIn;
        ins->scene_at = ins->scene_next;
        ins->scene_time = 0;
      } else {
        ins->scene_mode = Incenter_SceneMode_Passive;
      }
    }
  }

  // DRaw the transition
  switch (ins->scene_mode) 
  {
    case Incenter_SceneMode_TransitioningOut: pattern = pattern_sun_transition_shrink; break;
    case Incenter_SceneMode_TransitioningIn:  pattern = pattern_sun_transition_grow; break;
    default: {
      Incenter_Scene scene = ins->scenes[ins->scene_at];
      pattern = scene.patterns[ins->scene_mode];
    } break;
  }
  
  Assembly_Array assemblies = state->assemblies;
  if (pattern) 
  {
    pattern(assemblies.pixel_buffers[0], assemblies.strip_arrays[0], ins);
  } 
  else 
  {
    pattern_color(assemblies.pixel_buffers[0], assemblies.strip_arrays[0], 0, 0, 0);
  }
}

////////////////////////////////////////////////
// INCENTER LIFECYCLE

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
  // create user space data
  Incenter_State* ins = allocator_alloc_struct(permanent, Incenter_State);
  state->user_space_data = (u8*)ins;
  
  incenter_scenes_init(ins, 8, permanent);

  // create the sculpture
  u32 lights_per_primary_city = 123;
  u32 primary_city_lights = (city_count + 1) * lights_per_primary_city;
  u32 secondary_city_count = (city_secondary_count - city_secondary_first) + 1;
  u32 secondary_city_lights = secondary_city_count;
  u32 lights_cap = primary_city_lights + secondary_city_lights;
  Assembly_Handle ah = assembly_add(&state->assemblies, lit_str("incenter"), lights_cap, city_count + 2);  
  
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
  
  // ADDING PRIMARY CITIES
  r32 radius = INCENTER_FEET(10);
  for (u32 i = 0; i < city_count; i++)
  {
    Incenter_City_Desc city = city_descs[i];
    v3 end_p = incenter_latlng_to_cartesian(city.lat, city.lon, radius).xyz;

    Assembly_Strip* strip = assembly_add_strip(&state->assemblies, ah, 123);
    strip->output_kind = OutputData_NetworkSACN;
    strip->sacn_universe = city.sacn_universe;
    
    assembly_strip_create_leds(&state->assemblies, ah, strip, start_p, end_p, 123);
  }
  
  // ADDING SECONDARY CITIES
  // TODO: This probably isn't how these lights will be hooked up
  Assembly_Strip* secondary_strip = assembly_add_strip(&state->assemblies, ah, secondary_city_lights);
  secondary_strip->output_kind = OutputData_NetworkSACN;
  secondary_strip->sacn_universe = incenter_secondary_city_universe;
  for (u32 i = city_secondary_first; i < city_secondary_count; i++)
  {
    Incenter_City_Desc city = city_descs[i];
    v4 light_p = incenter_latlng_to_cartesian(city.lat, city.lon, radius);    
    assembly_add_led(&state->assemblies, ah, secondary_strip, light_p);
  }
  
  // PATTERN INIT
  pattern_random_fill_prep();

  r32 rad = 0.05f;
  sculpture_updated(state, 5, rad);
  scratch_release(scratch);
}

internal void
incenter_frame_prepare(App_State* state)
{
  
}

internal void
incenter_sculpture_visualizer_ui(App_State* state, Editor* ed)
{
  UI_Layout layout = *ed->ui.layout;
  layout.mode = UILayout_Columns,
  layout.bounds_min = (v2){0, 0},
  layout.bounds_max.x = 250;
  ui_layout_push(&ed->ui, &layout);

  Incenter_State* ins = (Incenter_State*)state->user_space_data;
  ui_text_f(&ed->ui, WHITE_V4, "Scene Time: %fs", ins->scene_time);
  ui_text_f(&ed->ui, WHITE_V4, "Scene: %s", ins->scenes[ins->scene_at].name);

  ui_layout_pop(&ed->ui);
}

internal void
incenter_frame(App_State* state)
{
  Incenter_State* ins = (Incenter_State*)state->user_space_data;
  Assembly_Array assemblies = state->assemblies;
    
  { // INPUT HANDLING
    Input_State* is = state->input_state;
    if (input_key_went_down(is, KeyCode_LeftArrow))  incenter_scene_go_to_prev(ins);
    if (input_key_went_down(is, KeyCode_RightArrow)) incenter_scene_go_to_next(ins);
  }

  ins->scene_time += state->target_seconds_per_frame;
  ins->transition_time += state->target_seconds_per_frame;
  incenter_scene_render(state, ins);
}

internal void
incenter_cleanup(App_State* state)
{
  
}