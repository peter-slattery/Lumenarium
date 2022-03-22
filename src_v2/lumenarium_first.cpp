#include "lumenarium_first.h"

internal App_State*
lumenarium_init()
{
  permanent = bump_allocator_create_reserve(MB(64));
  scratch = bump_allocator_create_reserve(MB(64));
  
  run_tests();
  
  App_State* state = allocator_alloc_struct(permanent, App_State);
  add_flag(state->flags, AppState_IsRunning);
  
  state->input_state = input_state_create();
  
  en_init(state);
  if (!has_flag(state->flags, AppState_NoEditor))
  {
    ed_init(state);
  }
  
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
}

internal void
lumenarium_frame(App_State* state)
{
  en_frame(state);
  if (!has_flag(state->flags, AppState_NoEditor))
  {
    ed_frame(state);
  }
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
  en_cleanup(state);
  if (!has_flag(state->flags, AppState_NoEditor))
  {
    ed_cleanup(state);
  }
}
