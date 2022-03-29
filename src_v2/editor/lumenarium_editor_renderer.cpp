

internal void
edr_render(App_State* state)
{
  Platform_Graphics_Frame_Desc desc = {};
  desc.clear_color = { 0.1f, 0.1f, 0.1f, 1 };
  desc.viewport_min = { 0, 0 };
  desc.viewport_max = state->editor->window_dim;
  platform_frame_begin(desc);
  platform_frame_clear();
  
  platform_geometry_bind(state->editor->renderer.geo);
  platform_texture_bind(state->editor->renderer.tex);
  platform_shader_bind(state->editor->renderer.shd);
  platform_geometry_draw(state->editor->renderer.geo);
}