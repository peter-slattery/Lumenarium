
#define INPUT_FRAME_STRING_LENGTH 32
struct Input_Frame
{
  Platform_Key_Flags key_flags[KeyCode_Count];
  
  char string_input[INPUT_FRAME_STRING_LENGTH];
  u32  string_input_len;
  
  v2 mouse_pos;
  s32 mouse_scroll;
};

struct Input_State
{
  Input_Frame frames[2];
  Input_Frame* frame_hot;
  Input_Frame* frame_cold;
  
  // cross frame state tracking
  v2 mouse_pos_delta;
  v2 mouse_pos_down;
};

#define key_was_down(key_flags)   has_flag((key_flags), KeyFlag_State_WasDown)
#define key_is_down(key_flags)    has_flag((key_flags), KeyFlag_State_IsDown)
#define key_was_up(key_flags)   (!has_flag((key_flags), KeyFlag_State_WasDown)
#define key_is_up(key_flags)    (!has_flag((key_flags), KeyFlag_State_IsDown)

internal Input_State
input_state_create()
{
  Input_State result = {};
  result.frame_hot  = result.frames + 0;
  result.frame_cold = result.frames + 1;
  return result;
}

internal void
input_state_swap_frames(Input_State* input_state)
{
  Input_Frame* next_hot = input_state->frame_cold;
  input_state->frame_cold = input_state->frame_hot;
  input_state->frame_hot = next_hot;
  
  // Clear the new hot input frame
  Platform_Key_Flags* hot_key_flags = input_state->frame_hot->key_flags;
  Platform_Key_Flags* cold_key_flags = input_state->frame_cold->key_flags;
  for (u32 i = 0; i < KeyCode_Count; i++) hot_key_flags[i] = cold_key_flags[i];
  input_state->frame_hot->string_input_len = 0;
}

// Key State Queries

internal bool 
input_key_is_down(Input_State* input_state, Platform_Key_Code key)
{
  Platform_Key_Flags flags = input_state->frame_hot->key_flags[key];
  return key_is_down(flags);
}

internal bool 
input_key_went_down(Input_State* input_state, Platform_Key_Code key)
{
  Platform_Key_Flags flags = input_state->frame_hot->key_flags[key];
  return key_is_down(flags) && !key_was_down(flags);
}

internal bool 
input_key_is_up(Input_State* input_state, Platform_Key_Code key)
{
  Platform_Key_Flags flags = input_state->frame_hot->key_flags[key];
  return !key_is_down(flags);
}

internal bool 
input_key_went_up(Input_State* input_state, Platform_Key_Code key)
{
  Platform_Key_Flags flags = input_state->frame_hot->key_flags[key];
  return !key_is_down(flags) && key_was_down(flags);
}
