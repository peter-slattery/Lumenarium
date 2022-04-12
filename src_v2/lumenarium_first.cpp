#include "lumenarium_first.h"
#include "user_space/user_space_incenter.cpp"

internal App_State*
lumenarium_init()
{
  App_State* state = 0;
  
  permanent = bump_allocator_create_reserve(GB(2));
  scratch_ = bump_allocator_create_reserve(GB(8));
  platform_file_jobs_init();
  
  run_tests();
  scratch_get(scratch);
  App_Init_Desc desc = incenter_get_init_desc();
  // TODO(PS): make sure the values make sense in desc
  
  state = allocator_alloc_struct(permanent, App_State);
  add_flag(state->flags, AppState_IsRunning);
  add_flag(state->flags, AppState_RunEditor);
  
  state->input_state = input_state_create(permanent);
  
  String exe_file_path = platform_get_exe_path(scratch.a);
  u64 run_tree_start = string_find_substring(exe_file_path, lit_str("run_tree"), 0, StringMatch_FindLast);
  u64 run_tree_end = run_tree_start + lit_str("run_tree").len;
  String run_tree_path = string_get_prefix(exe_file_path, run_tree_end);
  String run_tree_path_nullterm = string_copy(run_tree_path, scratch.a);
  platform_pwd_set(run_tree_path_nullterm);
  
  en_init(state, desc);
  if (has_flag(state->flags, AppState_RunEditor))
  {
    ed_init(state);
  }
  incenter_init(state);
  return state;
}

internal void
lumenarium_frame_prepare(App_State* state)
{
  allocator_clear(scratch_);
  
  input_state_swap_frames(state->input_state);
  
  en_frame_prepare(state);
  if (has_flag(state->flags, AppState_RunEditor))
  {
    ed_frame_prepare(state);
  }
  incenter_frame_prepare(state);
  
  platform_file_async_jobs_do_work(4, (u8*)state);
}

internal void
lumenarium_frame(App_State* state)
{
  en_frame(state);
  if (has_flag(state->flags, AppState_RunEditor))
  {
    ed_frame(state);
  }
  incenter_frame(state);
}

internal void
lumenarium_event(Platform_Window_Event evt, App_State* state)
{
  Input_Frame* frame = state->input_state->frame_hot;
  switch (evt.kind)
  {
    case WindowEvent_MouseScroll:
    {
      frame->mouse_scroll = evt.scroll_amt;
    } break;
    
    case WindowEvent_MouseMoved:
    {
      v2 mouse_pos_old = frame->mouse_pos;
      frame->mouse_pos = v2{ (r32)evt.mouse_x, (r32)evt.mouse_y };
      state->input_state->mouse_pos_delta = frame->mouse_pos - mouse_pos_old;
    } break;
    
    case WindowEvent_ButtonDown:
    case WindowEvent_ButtonUp:
    {
      frame->key_flags[evt.key_code] = evt.key_flags;
      if (evt.key_code == KeyCode_MouseLeftButton)
      {
        state->input_state->mouse_pos_down = v2{ 
          (r32)evt.mouse_x, (r32)evt.mouse_y 
        };
      }
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
  if (has_flag(state->flags, AppState_RunEditor))
  {
    ed_cleanup(state);
  }
}
