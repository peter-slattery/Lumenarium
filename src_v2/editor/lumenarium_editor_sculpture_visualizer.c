#include "lumenarium_editor_sculpture_visualizer_shaders.h"

internal void
ed_sculpture_visualizer_init(App_State* state)
{
  Editor* editor = state->editor;
  
  String vert = xplatform_shader_program_get_vert(sculpture_shd);
  String frag = xplatform_shader_program_get_frag(sculpture_shd);
  
  String attrs[] = { lit_str("a_pos"), lit_str("a_uv") };
  String uniforms[] = { lit_str("proj") };
  editor->sculpture_shd = shader_create(vert, frag, attrs, 2, uniforms, 1);
}

r32 cam_theta = 0;
u32 offset = 0;

internal void
ed_sculpture_visualizer(App_State* state)
{
  Editor* ed = state->editor;
  
  Input_State* in = state->input_state;
  u32 delta = 1;
  if (input_key_is_down(in, KeyCode_LeftShift) || input_key_is_down(in, KeyCode_RightShift))
  {
    delta = 100;
  }
  if (input_key_went_down(in, KeyCode_UpArrow))
  {
    offset += delta;
    printf("%d\n", offset);
  }
  if (input_key_went_down(in, KeyCode_DownArrow))
  {
    offset -= delta;
    printf("%d\n", offset);
  }
  offset = clamp(0, offset, ed->sculpture_geo.indices_len);


  // Set the viewport to the current layout's region so that the sculpture
  // never overlaps any other ui elements
  UI_Layout l = *ed->ui.layout;
  v2 view_dim = HMM_SubtractVec2(l.bounds_max, l.bounds_min);
  v2 view_min = l.bounds_min;
  v2 view_max = l.bounds_max;
  v2 view_min_scaled = HMM_MultiplyVec2(view_min, ed->content_scale);
  v2 view_dim_scaled = HMM_MultiplyVec2(view_dim, ed->content_scale);
  glViewport(
    (s32)view_min_scaled.x, 
    (s32)view_min_scaled.y, 
    (u32)view_dim_scaled.x, 
    (u32)view_dim_scaled.y
  );
  
  // TODO(PS): TEMPORARY CAMERA CODE
  cam_theta += 0.01f;
  r32 cam_r = 100;
  v3 camera_pos = (v3){sinf(cam_theta) * cam_r, 25, cosf(cam_theta) * cam_r};
  r32 aspect = view_dim.x / view_dim.y;
  m44 proj = HMM_Perspective(45.0, aspect, 0.01f, 500);
  m44 view = HMM_LookAt(camera_pos, (v3){0,0,0}, (v3){0,1,0});
  
  shader_bind(ed->sculpture_shd);
  set_uniform(ed->sculpture_shd, 0, HMM_MultiplyMat4(proj, view));
  texture_bind(ed->sculpture_tex);
  geometry_bind(ed->sculpture_geo);

  u32 i = 1008;
  u32 j = 2868;
  u32 k = ed->sculpture_geo.indices_len;
  u32 h = (i * 6) + 3;
  geometry_drawi(ed->sculpture_geo, k, 0);
  
  // reset the viewport for all other rendering
  v2 wds = HMM_MultiplyVec2(ed->window_dim, ed->content_scale);
  glViewport(0, 0, (s32)wds.x, (s32)wds.y);
}