#include "lumenarium_first.h"
#include "user_space/user_space_incenter.cpp"

Platform_Geometry_Buffer quad0;
Platform_Shader shader0;

static r32 z_ = 0;
static r32 r_ = 0.3f;
static r32 quad_verts[] = {
  -r_,  -r_,  z_,  1.0f,
  r_, -r_,  z_,  1.0f,
  r_, r_,  z_,  1.0f,
  -r_,  r_,  z_,  1.0f,
};

static u32 quad_indices[] = {
  0, 1, 2,
  0, 2, 3,
};

static String shader_code_vert_win32 = lit_str(
                                               "#version 330 core\n"
                                               "layout (location = 0) in  vec4 coordinates;\n"
                                               "void main(void) {\n"
                                               "  gl_Position = coordinates;\n"
                                               "}"
                                               );

static String shader_code_vert_wasm = lit_str(
                                              "attribute vec4 coordinates;\n"
                                              "void main(void) {\n"
                                              "  gl_Position = coordinates;\n"
                                              "}");

static String shader_code_frag_win32 = lit_str(
                                               "#version 330 core\n"
                                               "out vec4 FragColor;\n"
                                               "void main(void) {\n"
                                               "  FragColor = vec4(1,0,1,1);\n"
                                               "}"
                                               );

static String shader_code_frag_wasm = lit_str(
                                              "void main(void) {\n"
                                              "  gl_FragColor = vec4(1, 0, 1, 1);\n"
                                              "}");

void make_quad()
{
  // TODO(PS): TEMP
#if defined(PLATFORM_win32)
  String shader_code_vert = shader_code_vert_win32;
  String shader_code_frag = shader_code_frag_win32;
#elif defined(PLATFORM_wasm)
  String shader_code_vert = shader_code_vert_wasm;
  String shader_code_frag = shader_code_frag_wasm;
#endif
  
  quad0 = platform_geometry_buffer_create(
                                          quad_verts, 16, quad_indices, 6
                                          );
  
  String attribs[] = { lit_str("coordinates") };
  shader0 = platform_shader_create(
                                   shader_code_vert, shader_code_frag, attribs, 1
                                   );
  
  platform_vertex_attrib_pointer(quad0, shader0, 0);
}

internal App_State*
lumenarium_init()
{
  App_State* state = 0;
  
  permanent = bump_allocator_create_reserve(MB(4));
  scratch = bump_allocator_create_reserve(KB(64));
  
  
  run_tests();
  
  App_Init_Desc desc = incenter_get_init_desc();
  // TODO(PS): make sure the values make sense in desc
  
  state = allocator_alloc_struct(permanent, App_State);
  add_flag(state->flags, AppState_IsRunning);
  
  state->input_state = input_state_create();
  
  en_init(state, desc);
  if (!has_flag(state->flags, AppState_NoEditor))
  {
    ed_init(state);
  }
  incenter_init(state);
  
  
  make_quad();
  
  return state;
}

internal void
lumenarium_frame_prepare(App_State* state)
{
  allocator_clear(scratch);
  
  input_state_swap_frames(&state->input_state);
  
  en_frame_prepare(state);
  if (!has_flag(state->flags, AppState_NoEditor))
  {
    ed_frame_prepare(state);
  }
  incenter_frame_prepare(state);
}

internal void
lumenarium_frame(App_State* state)
{
  en_frame(state);
  if (!has_flag(state->flags, AppState_NoEditor))
  {
    //ed_frame(state);
    
    Platform_Graphics_Frame_Desc desc = {};
    desc.clear_color = { 0.1f, 0.1f, 0.1f, 1 };
    desc.viewport_min = { 0, 0 };
    desc.viewport_max = { 1600, 900 };
    platform_frame_begin(desc);
    platform_frame_clear();
    
    
    platform_geometry_bind(quad0);
    platform_shader_bind(shader0);
    platform_geometry_draw(quad0);
    
    
  }
  incenter_frame(state);
}

internal void
lumenarium_event(Platform_Window_Event evt, App_State* state)
{
  Input_Frame* frame = state->input_state.frame_hot;
  switch (evt.kind)
  {
    case WindowEvent_MouseScroll:
    {
      frame->mouse_scroll = evt.scroll_amt;
    } break;
    
    case WindowEvent_ButtonDown:
    case WindowEvent_ButtonUp:
    {
      frame->key_flags[evt.key_code] = evt.key_flags;
    } break;
    
    case WindowEvent_Char:
    {
      frame->string_input[frame->string_input_len++] = evt.char_value;
    } break;
    
    case WindowEvent_WindowClosed:
    {
      rem_flag(state->flags, AppState_IsRunning);
    } break;
    
    invalid_default_case;
  }
}

internal void
lumenarium_cleanup(App_State* state)
{
  incenter_cleanup(state);
  en_cleanup(state);
  if (!has_flag(state->flags, AppState_NoEditor))
  {
    ed_cleanup(state);
  }
}
