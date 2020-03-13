//
// File: gs_input.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef GS_INPUT_H
enum key_code
{
    KeyCode_Invalid,
    
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

internal char
CharacterFromKeyCode (key_code Code)
{
    char Result = 0;
    
    switch (Code)
    {
        case KeyCode_Space: { Result = ' '; }break;
        case KeyCode_Tab: { Result = '\t'; }break;
        
        // Letters
        case KeyCode_a: { Result = 'a'; }break; 
        case KeyCode_b: { Result = 'b'; }break; 
        case KeyCode_c: { Result = 'c'; }break; 
        case KeyCode_d: { Result = 'd'; }break;
        case KeyCode_e: { Result = 'e'; }break;
		case KeyCode_f: { Result = 'f'; }break;
		case KeyCode_g: { Result = 'g'; }break;
		case KeyCode_h: { Result = 'h'; }break;
        case KeyCode_i: { Result = 'i'; }break;
		case KeyCode_j: { Result = 'j'; }break;
		case KeyCode_k: { Result = 'k'; }break;
		case KeyCode_l: { Result = 'l'; }break;
		case KeyCode_m: { Result = 'm'; }break;
		case KeyCode_n: { Result = 'n'; }break;
		case KeyCode_o: { Result = 'o'; }break;
		case KeyCode_p: { Result = 'p'; }break; 
        case KeyCode_q: { Result = 'q'; }break;
		case KeyCode_r: { Result = 'r'; }break;
		case KeyCode_s: { Result = 's'; }break;
		case KeyCode_t: { Result = 't'; }break;
		case KeyCode_u: { Result = 'u'; }break;
		case KeyCode_v: { Result = 'v'; }break;
		case KeyCode_w: { Result = 'w'; }break;
		case KeyCode_x: { Result = 'x'; }break; 
        case KeyCode_y: { Result = 'y'; }break;
		case KeyCode_z: { Result = 'z'; }break;
        
        case KeyCode_A: { Result = 'A'; }break;
		case KeyCode_B: { Result = 'B'; }break;
		case KeyCode_C: { Result = 'C'; }break;
		case KeyCode_D: { Result = 'D'; }break;
		case KeyCode_E: { Result = 'E'; }break;
		case KeyCode_F: { Result = 'F'; }break;
		case KeyCode_G: { Result = 'G'; }break;
		case KeyCode_H: { Result = 'H'; }break;
        case KeyCode_I: { Result = 'I'; }break;
		case KeyCode_J: { Result = 'J'; }break;
		case KeyCode_K: { Result = 'K'; }break;
		case KeyCode_L: { Result = 'L'; }break;
		case KeyCode_M: { Result = 'M'; }break;
		case KeyCode_N: { Result = 'N'; }break;
		case KeyCode_O: { Result = 'O'; }break;
		case KeyCode_P: { Result = 'P'; }break; 
        case KeyCode_Q: { Result = 'Q'; }break;
		case KeyCode_R: { Result = 'R'; }break;
		case KeyCode_S: { Result = 'S'; }break;
		case KeyCode_T: { Result = 'T'; }break;
		case KeyCode_U: { Result = 'U'; }break;
		case KeyCode_V: { Result = 'V'; }break;
		case KeyCode_W: { Result = 'W'; }break;
		case KeyCode_X: { Result = 'X'; }break; 
        case KeyCode_Y: { Result = 'Y'; }break;
		case KeyCode_Z: { Result = 'Z'; }break;
        
        // Numbers
        case KeyCode_0: { Result = '0'; }break;
		case KeyCode_1: { Result = '1'; }break;
		case KeyCode_2: { Result = '2'; }break;
		case KeyCode_3: { Result = '3'; }break;
		case KeyCode_4: { Result = '4'; }break;
		case KeyCode_5: { Result = '5'; }break;
		case KeyCode_6: { Result = '6'; }break;
		case KeyCode_7: { Result = '7'; }break;
        case KeyCode_8: { Result = '8'; }break;
		case KeyCode_9: { Result = '9'; }break;
        
        case KeyCode_Num0: { Result = '0'; }break;
		case KeyCode_Num1: { Result = '1'; }break;
		case KeyCode_Num2: { Result = '2'; }break;
		case KeyCode_Num3: { Result = '3'; }break;
		case KeyCode_Num4: { Result = '4'; }break;
		case KeyCode_Num5: { Result = '5'; }break; 
        case KeyCode_Num6: { Result = '6'; }break;
		case KeyCode_Num7: { Result = '7'; }break;
		case KeyCode_Num8: { Result = '8'; }break;
		case KeyCode_Num9: { Result = '9'; }break;
        
        // Symbols
        case KeyCode_Bang: { Result = '!'; }break;
		case KeyCode_At: { Result = '@'; }break;
		case KeyCode_Pound: { Result = '#'; }break;
		case KeyCode_Dollar: { Result = '$'; }break;
		case KeyCode_Percent: { Result = '%'; }break;
		case KeyCode_Carrot: { Result = '^'; }break;
        case KeyCode_Ampersand: { Result = '&'; }break;
		case KeyCode_Star: { Result = '*'; }break;
		case KeyCode_LeftParen: { Result = '('; }break;
		case KeyCode_RightParen: { Result = ')'; }break;
		case KeyCode_Minus: { Result = '-'; }break;
		case KeyCode_Plus: { Result = '+'; }break;
        case KeyCode_Equals: { Result = '='; }break;
		case KeyCode_Underscore: { Result = '_'; }break;
		case KeyCode_LeftBrace: { Result = '['; }break;
		case KeyCode_RightBrace: { Result = ']'; }break;
		case KeyCode_LeftBracket: { Result = '{'; }break;
        case KeyCode_RightBracket: { Result = '}'; }break;
		case KeyCode_Colon: { Result = ':'; }break;
		case KeyCode_SemiColon: { Result = ';'; }break;
		case KeyCode_SingleQuote: { Result = '\''; }break;
		case KeyCode_DoubleQuote: { Result = '"'; }break;
        case KeyCode_ForwardSlash: { Result = '/'; }break;
		case KeyCode_Backslash: { Result = '\\'; }break;
		case KeyCode_Pipe: { Result = '|'; }break;
		case KeyCode_Comma: { Result = ','; }break;
		case KeyCode_Period: { Result = '.'; }break; 
        case KeyCode_QuestionMark: { Result = '?'; }break;
		case KeyCode_LessThan: { Result = '<'; }break;
		case KeyCode_GreaterThan: { Result = '>'; }break;
		case KeyCode_Tilde: { Result = '~'; }break;
		case KeyCode_BackQuote: { Result = '`'; }break;
    }
    
    Assert(Result != 0);
    return Result;
}

enum modifier_flags
{
    Modifier_Shift = 1 << 0,
    Modifier_Ctrl = 1 << 1,
    Modifier_Alt = 1 << 2,
    Modifier_Sys = 1 << 3, // NOTE(Peter): this is the windows key
};

#define INPUT_FRAME_STRING_LENGTH 32
struct input_frame
{
    b32 KeysDown[(int)KeyCode_Count];
    s32 StringInputUsed;
    char StringInput[INPUT_FRAME_STRING_LENGTH];
    s32 MouseX, MouseY, MouseScroll;
};

struct input
{
    input_frame Frames[2];
    input_frame* New;
    input_frame* Old;
    s32 MouseDownX, MouseDownY;
};

enum key_state_flags
{
    KeyState_WasDown = 1 << 0,
    KeyState_IsDown = 1 << 1,
};

#define KeyWasDown(event) ((event & KeyState_WasDown) > 0)
#define KeyIsDown(event) ((event & KeyState_IsDown) > 0)

struct input_entry
{
    key_code Key;
    b32 State;
    b32 Modifiers;
};

struct input_queue
{
    s32 QueueSize;
    s32 QueueUsed;
    input_entry* Entries;
};

enum cursor_type
{
    CursorType_Arrow,
    CursorType_Pointer,
    CursorType_Loading,
    CursorType_HorizontalArrows,
    CursorType_VerticalArrows,
    CursorType_DiagonalTopLeftArrows,
    CursorType_DiagonalTopRightArrows,
    CursorType_Count,
};

struct mouse_state
{
    s32 Scroll;
    
    v2 Pos;
    v2 OldPos;
    v2 DeltaPos;
    v2 DownPos;
    
    b32 LeftButtonState;
    b32 MiddleButtonState;
    b32 RightButtonState;
    
    cursor_type CursorType;
};

internal input_queue
InitializeInputQueue (u8* Memory, s32 MemorySize)
{
    input_queue Result = {};
    s32 EntriesCount = MemorySize / sizeof(input_entry);
    Result.QueueSize = EntriesCount;
    Result.QueueUsed = 0;
    Result.Entries = (input_entry*)Memory;
    return Result;
}

internal void
ResetInputQueue (input_queue* Queue)
{
    Queue->QueueUsed = 0;
}

internal void InitializeInput (input* Input);
internal void SwapInputBuffers (input* Input);

internal void
InitializeInput (input* Input)
{
    *(Input) = {};
    Input->New = &Input->Frames[0];
    Input->Old = &Input->Frames[1];
}

internal void
SwapInputBuffers (input* Input)
{
    input_frame* NowOld = Input->New;
    Input->New = Input->Old;
    Input->Old = NowOld;
    
    for (s32 Key = 0; Key < KeyCode_Count; Key++) { Input->New->KeysDown[Key] = Input->Old->KeysDown[Key]; }
    Input->New->StringInputUsed = 0;
}

internal b32
KeyDown (input Input, key_code Key)
{
    return Input.New->KeysDown[Key];
}

internal b32
KeyTransitionedDown (input Input, key_code Key)
{
    return Input.New->KeysDown[Key] && !Input.Old->KeysDown[Key];
}

internal b32
KeyTransitionedUp (input Input, key_code Key)
{
    return !Input.New->KeysDown[Key] && Input.Old->KeysDown[Key];
}

internal void
AddInputEventEntry (input_queue* Queue, key_code Key, 
                    b32 WasDown, b32 IsDown, b32 ShiftDown, b32 AltDown, b32 CtrlDown, b32 SysDown)
{
    Assert(Queue->QueueUsed < Queue->QueueSize);
    
    input_entry* Entry = Queue->Entries + Queue->QueueUsed++;
    Entry->Key = Key;
    
    Entry->State = (key_state_flags)0;
    if (WasDown) { Entry->State |= KeyState_WasDown; }
    if (IsDown) { Entry->State |= KeyState_IsDown; }
    
    Entry->Modifiers = 0;
    if (ShiftDown) { Entry->Modifiers |= Modifier_Shift; }
    if (CtrlDown) { Entry->Modifiers |= Modifier_Ctrl; }
    if (AltDown) { Entry->Modifiers |= Modifier_Alt; }
    if (SysDown) { Entry->Modifiers |= Modifier_Sys; }
}

internal b32
KeyTransitionedDown (input_entry Entry)
{
    return (!KeyWasDown(Entry.State) && KeyIsDown(Entry.State));
}

internal b32
KeyTransitionedUp (input_entry Entry)
{
    return (KeyWasDown(Entry.State) && !KeyIsDown(Entry.State));
}

internal b32
KeyHeldDown (input_entry Entry)
{
    return (KeyWasDown(Entry.State) && KeyIsDown(Entry.State));
}

internal b32
MouseButtonTransitionedDown (b32 MouseButtonState)
{
    return (!KeyWasDown(MouseButtonState) && KeyIsDown(MouseButtonState));
}

internal b32
MouseButtonTransitionedUp (b32 MouseButtonState)
{
    return (KeyWasDown(MouseButtonState) && !KeyIsDown(MouseButtonState));
}

internal b32
MouseButtonHeldDown (b32 MouseButtonState)
{
    b32 WasDown = KeyWasDown(MouseButtonState);
    b32 IsDown = KeyIsDown(MouseButtonState);
    return (WasDown && IsDown);
}

inline b32
GetMouseButtonStateAdvanced (b32 ButtonState)
{
    b32 Result = ButtonState;
    if (ButtonState & KeyState_WasDown &&
        !((ButtonState & KeyState_IsDown) > 0))
    {
        Result= 0;
    } 
    else if (ButtonState & KeyState_IsDown) 
    { 
        Result |= KeyState_WasDown; 
    }
    return Result;
}

#define GS_INPUT_H
#endif // GS_INPUT_H