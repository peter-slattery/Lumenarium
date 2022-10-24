
#define SECONDARY_CITY_CAP 50
u32 secondary_strips_len = 0;
Assembly_Strip* secondary_city_strips[SECONDARY_CITY_CAP];

#include "incenter_scenes.h"
#include "incenter_patterns.c"
#include "incenter_secondary_patterns.c"
#include "incenter_scenes.c"
#include "incenter_live_answers.c"

////////////////////////////////////////////////
// INCENTER SCENES

internal void
incenter_scenes_init(Incenter_State* ins, u32 cap, Allocator* a)
{
  incenter_scene_descs_init();
  ins->scenes = incenter_scene_descs;
  ins->scenes_cap = Incenter_Scene_Count;
  ins->scene_at = Incenter_Scene_AnyoneHome;
  //Incenter_Scene_Question_LostAccessToResources;
}

internal void
incenter_reset_inputs(Incenter_State* ins) 
{
  // reset bar chart over time
  needs_reset = true;
  year_max = 0;
  month_max = 0;
  year_at = 0;
  month_at = 0;
  month_start_time = 0;
  
  // reset inputs
  ins->input_pct = 0.5f;
  ins->input_option = 0;
  ins->input_advance = false;
}

internal void
incenter_go_to_transitioning_in(Incenter_State* ins)
{
  if (ins->scene_next != Incenter_Scene_WelcomeHome ||
      ins->scene_at != Incenter_Scene_AnyoneHome) {
    ins->transition_time = 0;
    ins->scene_mode = Incenter_SceneMode_TransitioningIn;
    ins->scene_time = 0;
  } else {
    ins->scene_mode = Incenter_SceneMode_Passive;
  }
  ins->scene_at = ins->scene_next;
  incenter_reset_inputs(ins);
}

internal void
incenter_scene_go_to(Incenter_State* ins, u32 index)
{
  ins->transition_time = 0;
  ins->scene_next = index % ins->scenes_cap;
  ins->scene_mode = Incenter_SceneMode_TransitioningOut;
  if (ins->scene_at == Incenter_Scene_WelcomeHome) {
    incenter_go_to_transitioning_in(ins);
  }
  
  printf("Switching To: %d:%s\n", ins->scene_next, ins->scenes[ins->scene_next].name);
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
        incenter_go_to_transitioning_in(ins);
      } else {
        ins->scene_mode = Incenter_SceneMode_Input;
        if (ins->scene_next == Incenter_Scene_WelcomeHome ||
            ins->scene_next == Incenter_Scene_AnyoneHome) {
          ins->scene_mode = Incenter_SceneMode_Passive;
        }
      }
    }
  }
  
  // DRaw the transition
  switch (ins->scene_mode) 
  {
    case Incenter_SceneMode_TransitioningOut: pattern = pattern_sun_transition_shrink; break;
    case Incenter_SceneMode_TransitioningIn:  pattern = pattern_sun_transition_grow; break;
    case Incenter_SceneMode_Input: pattern = pattern_scene_input; break;
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

#include "incenter_interface_connection.h"
#include "incenter_interface_connection.c"

////////////////////////////////////////////////
// INCENTER LIFECYCLE

#define INCENTER_RADIUS INCENTER_FEET(10)
internal Assembly_Strip*
incenter_add_secondary_city_strip(Assembly_Array* assemblies, Assembly_Handle ah, u32 universe, u32 count, Incenter_City_Id city)
{
  Assembly_Strip* s = assembly_add_strip(assemblies, ah, count);
  s->output_kind = OutputData_NetworkSACN;
  s->sacn_universe = universe;
  
  Incenter_City_Desc d = city_descs[city];
  v4 city_p = incenter_latlng_to_cartesian(d.lat, d.lon, INCENTER_RADIUS);
  v4 end_p  = incenter_latlng_to_cartesian(d.lat, d.lon, INCENTER_RADIUS * 2);
  for (u32 i = 0; i < count; i++)
  {
    v4 p = pm_lerp_v4(city_p, (r32)i / (r32)count, end_p);
    assembly_add_led(assemblies, ah, s, p);
  }
  
  secondary_city_strips[secondary_strips_len++] = s;
  return s;
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
  u32 lights_cap = primary_city_lights + 800;
  Assembly_Handle ah = assembly_add(&state->assemblies, lit_str("incenter"), lights_cap, city_count + 50);  
  
  scratch_get(scratch);
  Allocator* s = scratch.a;
  
  v3 start_p = (v3){0, 0, 0};
  
  
  Assembly_Strip* brc = assembly_add_strip(&state->assemblies, ah, 140);
  brc->output_kind = OutputData_NetworkSACN;
  brc->sacn_universe = city_descs[city_black_rock].sacn_universe;
  // Arctic
  assembly_strip_append_leds(
    &state->assemblies, 
    ah,
    brc, 
    (v3){0, INCENTER_RADIUS, 0}, 
    start_p,
    90
  );
  // BRC
  assembly_strip_append_leds(
    &state->assemblies,
    ah,
    brc,
    start_p,
    (v3){0, INCENTER_FEET(-4.5f), 0},
    50
  );
  
  // ADDING PRIMARY CITIES
  r32 radius = INCENTER_RADIUS;
  for (u32 i = 1; i < city_count; i++)
  {
    Incenter_City_Desc city = city_descs[i];
    if (city.sacn_universe == 0) {
      printf("skipping %s\n", city_strings[i]);
      continue;
    }
    
    v3 end_p = incenter_latlng_to_cartesian(city.lat, city.lon, radius).xyz;
    
    Assembly_Strip* strip = assembly_add_strip(&state->assemblies, ah, 123);
    strip->output_kind = OutputData_NetworkSACN;
    strip->sacn_universe = city.sacn_universe;
    
    assembly_strip_append_leds(&state->assemblies, ah, strip, end_p, start_p, 88);
  }
  
#define USING_ON_PLAYA_SECONDARY_CITIES 1
#if USING_ON_PLAYA_SECONDARY_CITIES
  // ADDING SECONDARY CITIES
  {
    for (u32 i = 0; i < SECONDARY_CITY_CAP; i++) secondary_city_strips[i] = 0;
    Assembly_Array* a = &state->assemblies;
    
    { // Australia      
      Assembly_Strip* aus_lef = incenter_add_secondary_city_strip(a, ah, 62, 10, city_brisbane);
      Assembly_Strip* aus_rig = incenter_add_secondary_city_strip(a, ah, 64, 8,  city_brisbane);
      Assembly_Strip* aus_cen = incenter_add_secondary_city_strip(a, ah, 34, 6,  city_brisbane);
    }
    { // Islands Above Australia 
      Assembly_Strip* indonesia1 = incenter_add_secondary_city_strip(a, ah, 44, 9, city_jakarta);
      Assembly_Strip* indonesia2 = incenter_add_secondary_city_strip(a, ah, 36, 9, city_jakarta);
      Assembly_Strip* north_left_asia = incenter_add_secondary_city_strip(a, ah, 54, 6, city_tokyo);
      Assembly_Strip* phillipines = incenter_add_secondary_city_strip(a, ah, 56, 10, city_jakarta);
    }
    { // East Asia
      // TODO(PS): update primary cities
      Assembly_Strip* cen_china_to_thailand = incenter_add_secondary_city_strip(a, ah, 37, 19, city_jakarta);
      Assembly_Strip* china_coast           = incenter_add_secondary_city_strip(a, ah, 38, 29, city_jakarta);
      Assembly_Strip* peninsula_below_china = incenter_add_secondary_city_strip(a, ah, 40, 10, city_jakarta);
      Assembly_Strip* mainland_right_of_japan_north = incenter_add_secondary_city_strip(a, ah, 40, 10, city_tokyo);
      Assembly_Strip* mainland_right_of_japan_south = incenter_add_secondary_city_strip(a, ah, 58, 9, city_tokyo);
    }
    { // Japan
      Assembly_Strip* japan = incenter_add_secondary_city_strip(a, ah, 60, 10, city_tokyo);
    }
    {
      Assembly_Strip* rig_south_africa = incenter_add_secondary_city_strip(a, ah, 196, 9, city_nairobi);
      Assembly_Strip* georgia      = incenter_add_secondary_city_strip(a, ah, 198, 10, city_tokyo); // TODO(PS): 
      Assembly_Strip* ethiopia     = incenter_add_secondary_city_strip(a, ah, 200,  5, city_addis_ababa);
      Assembly_Strip* turkey       = incenter_add_secondary_city_strip(a, ah, 202, 10, city_ankara);
      Assembly_Strip* sudan_egypt  = incenter_add_secondary_city_strip(a, ah, 204, 7, city_cairo);
      Assembly_Strip* iran         = incenter_add_secondary_city_strip(a, ah, 206,  9, city_tehran);
      Assembly_Strip* west_russia  = incenter_add_secondary_city_strip(a, ah, 208, 25, city_kyiv);
      Assembly_Strip* saudi_arabia = incenter_add_secondary_city_strip(a, ah, 212, 4, city_ankara);
      Assembly_Strip* south_africa = incenter_add_secondary_city_strip(a, ah, 214, 25, city_nairobi); // TODO(PS): 
      Assembly_Strip* right_india  = incenter_add_secondary_city_strip(a, ah, 215, 19, city_mumbai);
      Assembly_Strip* central_russia_right = incenter_add_secondary_city_strip(a, ah, 218, 7, city_kyiv);
      Assembly_Strip* left_africa  = incenter_add_secondary_city_strip(a, ah, 220, 7, city_nairobi);
      Assembly_Strip* the_stans    = incenter_add_secondary_city_strip(a, ah, 221, 19, city_tehran);
      Assembly_Strip* left_india   = incenter_add_secondary_city_strip(a, ah, 222, 40, city_mumbai);
    }
    
    {
      Assembly_Strip* italy      = incenter_add_secondary_city_strip(a, ah, 162, 9, city_paris);
      Assembly_Strip* nigeria_cameroon = incenter_add_secondary_city_strip(a, ah, 164, 6, city_abuja);
      Assembly_Strip* greece      = incenter_add_secondary_city_strip(a, ah, 166, 9, city_belgrade);
      Assembly_Strip* israel      = incenter_add_secondary_city_strip(a, ah, 168, 6, city_cairo);
      Assembly_Strip* central_europe = incenter_add_secondary_city_strip(a, ah, 170, 35, city_berlin);
      Assembly_Strip* libya = incenter_add_secondary_city_strip(a, ah, 172, 9, city_tunis);
      Assembly_Strip* peru = incenter_add_secondary_city_strip(a, ah, 182, 9, city_brasilia);
      Assembly_Strip* right_north_africa = incenter_add_secondary_city_strip(a, ah, 184, 13, city_rabat);
      Assembly_Strip* france = incenter_add_secondary_city_strip(a, ah, 188, 5, city_paris);
      Assembly_Strip* uk = incenter_add_secondary_city_strip(a, ah, 190, 6, city_paris);
      Assembly_Strip* scandanavia = incenter_add_secondary_city_strip(a, ah, 192, 5, city_berlin);
    }
    {
      Assembly_Strip* us_east_coast = incenter_add_secondary_city_strip(a, ah, 1, 20, city_washington);
      Assembly_Strip* central_americas = incenter_add_secondary_city_strip(a, ah, 2, 19, city_denver);
      Assembly_Strip* colombia = incenter_add_secondary_city_strip(a, ah, 6, 9, city_bogota);
      Assembly_Strip* missippi_river = incenter_add_secondary_city_strip(a, ah, 12, 9, city_washington);
      Assembly_Strip* chile_south = incenter_add_secondary_city_strip(a, ah, 14, 9, city_la_paz);
      Assembly_Strip* us_west_coast = incenter_add_secondary_city_strip(a, ah, 22, 17, city_san_francisco);
      Assembly_Strip* venezuela = incenter_add_secondary_city_strip(a, ah, 26, 5, city_bogota);
      Assembly_Strip* chile_north = incenter_add_secondary_city_strip(a, ah, 28, 10, city_la_paz);
      Assembly_Strip* left_canada = incenter_add_secondary_city_strip(a, ah, 32, 5, city_san_francisco);
    }
  }
  
#else
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
#endif
  
  // PATTERN INIT
  pattern_random_fill_prep();
  
  r32 rad = 0.05f;
  sculpture_updated(state, 5, rad);
  
  
  ins->running = true;
  incenter_interface_connection_init(state, ins);
  
  printf("Incenter Initialized\n");
  scratch_release(scratch);
}

internal void
incenter_frame_prepare(App_State* state)
{
  
}

#if defined(PLATFORM_SUPPORTS_EDITOR)
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
#endif

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
  
  incenter_interface_connection_frame(state, ins);
  
  ins->scene_time += state->target_seconds_per_frame * 0.5f;
  ins->transition_time += state->target_seconds_per_frame;
  incenter_scene_render(state, ins);
  
}

internal void
incenter_cleanup(App_State* state)
{
  Incenter_State* ins = (Incenter_State*)state->user_space_data;
  ins->running = false;
  os_thread_end(ins->interface_thread);
  incenter_interface_connection_cleanup(ins);
}

internal App_Init_Desc
user_space_get_init_desc()
{
  App_Init_Desc result = {
    .assembly_cap = 4,
    .init = incenter_init,
    .frame_prepare = incenter_frame_prepare,
#if defined(PLATFORM_SUPPORTS_EDITOR)
    .sculpture_visualizer_ui = incenter_sculpture_visualizer_ui,
#endif
    .frame = incenter_frame,
    .cleanup = incenter_cleanup,
  };
  return result;
}