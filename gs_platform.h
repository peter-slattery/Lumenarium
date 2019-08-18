#ifndef GS_PLATFORM_H

#ifndef GS_LANGUAGE_H
// Types
typedef signed char    b8;
typedef short int      b16;
typedef int            b32;
typedef long long int  b64;

typedef unsigned char          u8;
typedef unsigned short int     u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;

typedef signed char   s8;
typedef short int     s16;
typedef int           s32;
typedef long long int s64;

typedef float  r32;
typedef double r64;

#define internal static
#define local_persist static
#define global_variable static

#ifdef DEBUG
#define Assert(condition) if(!(condition)) { *((int *)0) = 5; }
#define InvalidCodePath Assert(0)
#define InvalidDefaultCase default: { InvalidCodePath; }
#endif

#endif

#ifdef PLATFORM_WINDOWS
#include "gs_win32.h"
#endif

struct window_info
{
    char* Name;
    s32 Width;
    s32 Height;
};

typedef struct window window;

#define PLATFORM_MEMORY_NO_ERROR 0
struct platform_memory_result
{
    u8* Base;
    s32 Size;
    s32 Error;
};

struct texture_buffer
{
    u8* Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
    s32 BytesPerPixel;
};

struct system_path
{
    char* Path;
    s32 PathLength;
    s32 IndexOfLastSlash;
};

#define PLATFORM_ALLOC(name) platform_memory_result name(s32 Size)
typedef PLATFORM_ALLOC(platform_alloc);

#define PLATFORM_FREE(name) b32 name(u8* Base, s32 Size)
typedef PLATFORM_FREE(platform_free);

#define PLATFORM_READ_ENTIRE_FILE(name) platform_memory_result name(char* Path)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char* Path, u8* Contents, s32 Size)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_GET_FILE_PATH(name) b32 name(char* PathBuffer, s32 BufferLength, const char* FilterStrings)
typedef PLATFORM_GET_FILE_PATH(platform_get_file_path);

#define PLATFORM_GET_GPU_TEXTURE_HANDLE(name) s32 name(u8* Memory, s32 Width, s32 Height)
typedef PLATFORM_GET_GPU_TEXTURE_HANDLE(platform_get_gpu_texture_handle);

typedef s32 platform_socket_handle;
typedef s32 platform_network_address_handle;

#define PLATFORM_GET_SOCKET_HANDLE(name) platform_socket_handle name(s32 AddressFamily, s32 Type, s32 Protocol) 
typedef PLATFORM_GET_SOCKET_HANDLE(platform_get_socket_handle);

#define PLATFORM_GET_SEND_ADDRESS_HANDLE(name) platform_network_address_handle name(s32 AddressFamily, u16 Port, u32 Address)
typedef PLATFORM_GET_SEND_ADDRESS_HANDLE(platform_get_send_address);

#define PLATFORM_SET_SOCKET_OPTION(name) s32 name(platform_socket_handle SocketHandle, s32 Level, s32 Option, const char* OptionValue, s32 OptionLength) 
typedef PLATFORM_SET_SOCKET_OPTION(platform_set_socket_option);

#define PLATFORM_SEND_TO(name) s32 name(platform_socket_handle SocketHandle, platform_network_address_handle AddressHandle, const char* Buffer, s32 BufferLength, s32 Flags)
typedef PLATFORM_SEND_TO(platform_send_to);

#define PLATFORM_CLOSE_SOCKET(name) void name(platform_socket_handle SocketHandle)
typedef PLATFORM_CLOSE_SOCKET(platform_close_socket);

#ifndef GS_INPUT
#define GS_INPUT
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

#endif // GS_INPUT

internal window PlatformCreateWindow (char* Name, s32 Width, s32 Height);


// Time

internal r32
GetSecondsElapsed (s64 Start, s64 End, s64 PerformanceCountFrequency)
{
    r32 Result = ((r32)(End - Start) / (r32) PerformanceCountFrequency);
    return Result;
}

#define GS_PLATFORM_H
#endif // GS_PLATFORM_H