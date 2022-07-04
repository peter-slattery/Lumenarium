#include "lumenarium_first.h"
#include "user_space/user_space_incenter.c"

void
sculpture_updated(App_State* state, r32 scale, r32 led_size)
{
  #if defined(PLATFORM_SUPPORTS_EDITOR)
    ed_sculpture_updated(state, scale, led_size);
  #endif
}

internal App_State*
lumenarium_init(Editor_Desc* ed_desc)
{
  App_State* state = 0;
  
  permanent = bump_allocator_create_reserve(GB(2));
  global_scratch_ = bump_allocator_create_reserve(GB(4));
  
  run_tests();
  
  //cvtcsv_convert(lit_str("./data/incenter_test_data_clean.csv"));  

  scratch_get(scratch);
  App_Init_Desc desc = incenter_get_init_desc();
  // TODO(PS): make sure the values make sense in desc
  
  state = allocator_alloc_struct(permanent, App_State);
  add_flag(state->flags, AppState_IsRunning);
  add_flag(state->flags, AppState_RunEditor);
  add_flag(state->flags, AppState_RunUserSpace);
  
  state->file_async_job_system = os_file_jobs_init();
  open_sockets_init();
  state->input_state = input_state_create(permanent);
  
  String exe_file_path = os_get_exe_path(scratch.a);
  u64 run_tree_start = string_find_substring(exe_file_path, lit_str("run_tree"), 0, StringMatch_FindLast);
  if (run_tree_start >= exe_file_path.cap)
  {
    u64 exe_path_start = string_find_substring(exe_file_path, lit_str("lumenarium"), 0, StringMatch_FindLast);
    if (exe_path_start < exe_file_path.cap)
    {
      String run_tree_path = string_get_prefix(exe_file_path, exe_path_start);
      String run_tree_path_nullterm = string_copy(run_tree_path, scratch.a);
      os_pwd_set(run_tree_path_nullterm);
    }
    else
    {
      printf("Unable to set working directory\n");
    }
  }
  else
  {
    u64 run_tree_end = run_tree_start + lit_str("run_tree").len;
    String run_tree_path = string_get_prefix(exe_file_path, run_tree_end);
    String run_tree_path_nullterm = string_copy(run_tree_path, scratch.a);
    os_pwd_set(run_tree_path_nullterm);
  }
  

  en_init(state, desc);
#if defined(PLATFORM_SUPPORTS_EDITOR)
  if (has_flag(state->flags, AppState_RunEditor)) ed_init(state, ed_desc);
#endif
  if (has_flag(state->flags, AppState_RunUserSpace)) incenter_init(state);
  scratch_release(scratch);
  return state;
}

internal void
lumenarium_frame_prepare(App_State* state)
{
  allocator_clear(global_scratch_);
  
  input_state_swap_frames(state->input_state);

  en_frame_prepare(state);
#if defined(PLATFORM_SUPPORTS_EDITOR)
  if (has_flag(state->flags, AppState_RunEditor)) ed_frame_prepare(state);
#endif
  if (has_flag(state->flags, AppState_RunUserSpace)) incenter_frame_prepare(state);
  
  file_async_jobs_do_work(&state->file_async_job_system, 4, (u8*)state);
}

internal void
lumenarium_frame(App_State* state)
{
  if (has_flag(state->flags, AppState_RunUserSpace)) incenter_frame(state);
  en_frame(state);
#if defined(PLATFORM_SUPPORTS_EDITOR)
  if (has_flag(state->flags, AppState_RunEditor)) ed_frame(state);
#endif
}

internal void
lumenarium_event(Window_Event evt, App_State* state)
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
      frame->mouse_pos = (v2){ (r32)evt.mouse_x, (r32)evt.mouse_y };
      state->input_state->mouse_pos_delta = HMM_SubtractVec2(frame->mouse_pos, mouse_pos_old);
    } break;
    
    case WindowEvent_ButtonDown:
    case WindowEvent_ButtonUp:
    {
      frame->key_flags[evt.key_code] = evt.key_flags;
      if (evt.key_code == KeyCode_MouseLeftButton)
      {
        state->input_state->mouse_pos_down = (v2){ 
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
  if (has_flag(state->flags, AppState_RunUserSpace)) incenter_cleanup(state);
  en_cleanup(state);
#if defined(PLATFORM_SUPPORTS_EDITOR)
  if (has_flag(state->flags, AppState_RunEditor)) ed_cleanup(state);
#endif
}
