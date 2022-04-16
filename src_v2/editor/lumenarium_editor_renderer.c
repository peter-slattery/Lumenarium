
internal void
edr_render_begin(App_State* state)
{
  Graphics_Frame_Desc desc = {};
  desc.clear_color = (v4){ 0.1f, 0.1f, 0.1f, 1 };
  desc.viewport_min = (v2){ 0, 0 };
  v2 wd = state->editor->window_dim;
  v2 cs = state->editor->content_scale;
  desc.viewport_max = HMM_MultiplyVec2(wd, cs);
  frame_begin(desc);
  frame_clear();
}

internal void
edr_render(App_State* state)
{
}