#if !defined(LUMENARIUM_CORE_WINDOW_H)
#define LUMENARIUM_CORE_WINDOW_H

typedef u32 Window_Event_Kind;
enum
{
  WindowEvent_Invalid = 0,
  
  WindowEvent_MouseScroll,
  WindowEvent_MouseMoved,
  WindowEvent_ButtonDown,
  WindowEvent_ButtonUp,
  WindowEvent_Char,
  WindowEvent_WindowClosed,
  
  WindowEvent_Count,
};

typedef u32 Key_Code;
enum
{
  KeyCode_Invalid = 0,
  
  KeyCode_Esc,
  
  KeyCode_Space,
  KeyCode_Tab,
  KeyCode_CapsLock,
  KeyCode_LeftShift, KeyCode_RightShift,
  KeyCode_LeftCtrl, KeyCode_RightCtrl,
  KeyCode_Fn,
  KeyCode_Alt,
  KeyCode_PageUp, KeyCode_PageDown,
  KeyCode_Backspace, KeyCode_Delete,
  KeyCode_Enter,
  
  // Function Keys
  KeyCode_F0, KeyCode_F1, KeyCode_F2, KeyCode_F3, KeyCode_F4, KeyCode_F5, KeyCode_F6, KeyCode_F7,
  KeyCode_F8, KeyCode_F9, KeyCode_F10, KeyCode_F11, KeyCode_F12,
  
  // Letters
  KeyCode_a, KeyCode_b, KeyCode_c, KeyCode_d, KeyCode_e, KeyCode_f, KeyCode_g, KeyCode_h,
  KeyCode_i, KeyCode_j, KeyCode_k, KeyCode_l, KeyCode_m, KeyCode_n, KeyCode_o, KeyCode_p,
  KeyCode_q, KeyCode_r, KeyCode_s, KeyCode_t, KeyCode_u, KeyCode_v, KeyCode_w, KeyCode_x,
  KeyCode_y, KeyCode_z,
  
  KeyCode_A, KeyCode_B, KeyCode_C, KeyCode_D, KeyCode_E, KeyCode_F, KeyCode_G, KeyCode_H,
  KeyCode_I, KeyCode_J, KeyCode_K, KeyCode_L, KeyCode_M, KeyCode_N, KeyCode_O, KeyCode_P,
  KeyCode_Q, KeyCode_R, KeyCode_S, KeyCode_T, KeyCode_U, KeyCode_V, KeyCode_W, KeyCode_X,
  KeyCode_Y, KeyCode_Z,
  
  // Numbers
  KeyCode_0, KeyCode_1, KeyCode_2, KeyCode_3, KeyCode_4, KeyCode_5, KeyCode_6, KeyCode_7,
  KeyCode_8, KeyCode_9,
  
  KeyCode_Num0, KeyCode_Num1, KeyCode_Num2, KeyCode_Num3, KeyCode_Num4, KeyCode_Num5,
  KeyCode_Num6, KeyCode_Num7, KeyCode_Num8, KeyCode_Num9,
  
  // Symbols
  KeyCode_Bang, KeyCode_At, KeyCode_Pound, KeyCode_Dollar, KeyCode_Percent, KeyCode_Carrot,
  KeyCode_Ampersand, KeyCode_Star, KeyCode_LeftParen, KeyCode_RightParen, KeyCode_Minus, KeyCode_Plus,
  KeyCode_Equals, KeyCode_Underscore, KeyCode_LeftBrace, KeyCode_RightBrace, KeyCode_LeftBracket,
  KeyCode_RightBracket, KeyCode_Colon, KeyCode_SemiColon, KeyCode_SingleQuote, KeyCode_DoubleQuote,
  KeyCode_ForwardSlash, KeyCode_Backslash, KeyCode_Pipe, KeyCode_Comma, KeyCode_Period,
  KeyCode_QuestionMark, KeyCode_LessThan, KeyCode_GreaterThan, KeyCode_Tilde, KeyCode_BackQuote,
  
  // Arrows
  KeyCode_UpArrow,
  KeyCode_DownArrow,
  KeyCode_LeftArrow,
  KeyCode_RightArrow,
  
  // Mouse
  // NOTE(Peter): Including this here so we can utilize the same KeyDown, KeyUp etc. functions
  KeyCode_MouseLeftButton,
  KeyCode_MouseMiddleButton,
  KeyCode_MouseRightButton,
  
  KeyCode_Count,
};

typedef u8 Key_Flags;
enum
{
  KeyFlag_None  = 0,
  
  KeyFlag_Mod_Shift = 1,
  KeyFlag_Mod_Ctrl  = 2,
  KeyFlag_Mod_Alt   = 4,
  KeyFlag_Mod_Sys   = 8,
  
  KeyFlag_State_WasDown = 16,
  KeyFlag_State_IsDown  = 32,
};

typedef struct Window_Event Window_Event;
struct Window_Event
{
  Window_Event_Kind kind;
  Key_Code          key_code;
  Key_Flags         key_flags;
  s32 mouse_x;
  s32 mouse_y;
  s32 scroll_amt;
  char char_value;
};

typedef u32 Cursor_Kind;
enum
{
  Cursor_Arrow,
  Cursor_Pointer,
  Cursor_Loading,
  Cursor_HArrows,
  Cursor_VArrows,
  Cursor_DTopLeftArrows,
  Cursor_DTopRightArrows,
  Cursor_Count,
};

#define INPUT_FRAME_STRING_LENGTH 32
typedef struct Input_Frame Input_Frame;
struct Input_Frame
{
  Key_Flags key_flags[KeyCode_Count];
  
  char string_input[INPUT_FRAME_STRING_LENGTH];
  u32  string_input_len;
  
  v2 mouse_pos;
  s32 mouse_scroll;
};

typedef struct Input_State Input_State;
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
#define key_was_up(key_flags)   (!has_flag((key_flags), KeyFlag_State_WasDown))
#define key_is_up(key_flags)    (!has_flag((key_flags), KeyFlag_State_IsDown))

internal Input_State* input_state_create(Allocator* a);
internal Key_Flags    input_key_advance(Key_Flags flag);
internal void         input_state_swap_frames(Input_State* input_state);
internal bool         input_key_is_down(Input_State* input_state, Key_Code key);
internal bool         input_key_went_down(Input_State* input_state, Key_Code key);
internal bool         input_key_is_up(Input_State* input_state, Key_Code key);
internal bool         input_key_went_up(Input_State* input_state, Key_Code key);

//////////////////////////////////////////
//////////////////////////////////////////
//         IMPLEMENTATION
//////////////////////////////////////////
//////////////////////////////////////////

internal Input_State*
input_state_create(Allocator* a)
{
  Input_State* result = allocator_alloc_struct(a, Input_State);
  result->frame_hot  = result->frames + 0;
  result->frame_cold = result->frames + 1;

  // Clear the new hot input frame
  Key_Flags* hot_key_flags = result->frame_hot->key_flags;
  Key_Flags* cold_key_flags = result->frame_cold->key_flags;
  for (u32 i = 0; i < KeyCode_Count; i++)
  {
    hot_key_flags[i] = 0;
    cold_key_flags[i] = 0;
  }
  return result;
}

internal Key_Flags
input_key_advance(Key_Flags flag)
{
  Key_Flags result = flag;
  if (key_is_down(flag)) 
  {
    add_flag(result, KeyFlag_State_WasDown);
  }
  if (key_is_up(flag)) 
  {
    rem_flag(result, KeyFlag_State_WasDown);
  }
  return result;
}

internal void
input_state_swap_frames(Input_State* input_state)
{
  Input_Frame* next_hot = input_state->frame_cold;
  input_state->frame_cold = input_state->frame_hot;
  input_state->frame_hot = next_hot;
  
  // Clear the new hot input frame
  Key_Flags* hot_key_flags = input_state->frame_hot->key_flags;
  Key_Flags* cold_key_flags = input_state->frame_cold->key_flags;
  for (u32 i = 0; i < KeyCode_Count; i++) 
  {
    hot_key_flags[i] = input_key_advance(cold_key_flags[i]);
  }
  input_state->frame_hot->string_input_len = 0;
}

// Key State Queries

internal bool 
input_key_is_down(Input_State* input_state, Key_Code key)
{
  Key_Flags flags = input_state->frame_hot->key_flags[key];
  return key_is_down(flags);
}

internal bool 
input_key_went_down(Input_State* input_state, Key_Code key)
{
  Key_Flags flags = input_state->frame_hot->key_flags[key];
  return key_is_down(flags) && !key_was_down(flags);
}

internal bool 
input_key_is_up(Input_State* input_state, Key_Code key)
{
  Key_Flags flags = input_state->frame_hot->key_flags[key];
  return !key_is_down(flags);
}

internal bool 
input_key_went_up(Input_State* input_state, Key_Code key)
{
  Key_Flags flags = input_state->frame_hot->key_flags[key];
  return !key_is_down(flags) && key_was_down(flags);
}
#endif // LUMENARIUM_CORE_WINDOW_H