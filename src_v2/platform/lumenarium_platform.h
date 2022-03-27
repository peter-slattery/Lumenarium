/* date = March 22nd 2022 2:05 am */

#ifndef LUMENARIUM_PLATFORM_H
#define LUMENARIUM_PLATFORM_H

// This is a file that defines the things that every platform
// must expose to the entire program

///////////////////////////////////////
//    Memory

u64  platform_page_size();

u8*  platform_mem_reserve(u64 size);
u8*  platform_mem_commit(u8* base, u64 size);
bool platform_mem_decommit(u8* base, u64 size);
bool platform_mem_release(u8* base, u64 size);

///////////////////////////////////////
//    File I/O

struct Platform_File_Handle
{
  u64 value;
};

typedef u32 Platform_File_Access_Flags;
enum
{
  FileAccess_None  = 0,
  FileAccess_Read  = 1,
  FileAccess_Write = 2,
};

typedef u32 Platform_File_Create_Flags;
enum
{
  // these match https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
  FileCreate_None = 0,
  FileCreate_New = 1,
  FileCreate_CreateAlways = 2,
  FileCreate_OpenExisting = 3,
  FileCreate_OpenAlways = 4,
};

typedef u32 Platform_File_Flags;
enum
{
  FileFlag_IsFile = 0,
  FileFlag_IsDir = 1,
};

struct Platform_File_Info
{
  String path;
  String path_abs;
  u64 size;
  u64 time_created;
  u64 time_last_write;
  Platform_File_Flags flags;
};

struct Platform_File_Info_List_Ele
{
  Platform_File_Info info;
  Platform_File_Info_List_Ele* next;
};

struct Platform_File_Info_List
{
  Platform_File_Info_List_Ele* first;
  Platform_File_Info_List_Ele* last;
};

Platform_File_Handle platform_file_open(String path, Platform_File_Access_Flags flags_access,  Platform_File_Create_Flags flags_create);
void                 platform_file_close(Platform_File_Handle file_handle);
Platform_File_Info   platform_file_get_info(Platform_File_Handle file_handle, Allocator* allocator);
Data platform_file_read_all(Platform_File_Handle file_handle, Allocator* allocator);
bool platform_file_write_all(Platform_File_Handle file_handle, Data file_data);
String platform_get_exe_path(Allocator* allocator);
bool platform_pwd_set(String path);

typedef u32 Platform_Enum_Dir_Flags;
enum
{
  EnumDir_Recursive = 1,
  EnumDir_IncludeDirectories = 2,
};

Platform_File_Info_List platform_dir_enum(String path, Platform_Enum_Dir_Flags flags, Allocator* allocator);

String platform_get_exe_path(Allocator* allocator);
bool   platform_pwd_set(String path);

///////////////////////////////////////
//    Windows & Events

enum Platform_Window_Event_Kind
{
  WindowEvent_Invalid = 0,
  
  WindowEvent_MouseScroll,
  WindowEvent_ButtonDown,
  WindowEvent_ButtonUp,
  WindowEvent_Char,
  WindowEvent_WindowClosed,
  
  WindowEvent_Count,
};

typedef u32 Platform_Key_Code;
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

typedef u8 Platform_Key_Flags;
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

struct Platform_Window_Event
{
  Platform_Window_Event_Kind kind;
  Platform_Key_Code          key_code;
  Platform_Key_Flags         key_flags;
  s32 mouse_x;
  s32 mouse_y;
  s32 scroll_amt;
  char char_value;
};

enum  Platform_Cursor_Kind
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

///////////////////////////////////////
//    Time

global r64 target_seconds_per_frame = 1.0 / 30.0f;

struct Platform_Ticks
{
  s64 value;
};

Platform_Ticks platform_get_ticks();
r64 platform_ticks_to_seconds(Platform_Ticks ticks);

Platform_Ticks
get_ticks_elapsed(Platform_Ticks start, Platform_Ticks end)
{
  Platform_Ticks result = {};
  result.value = end.value - start.value;
  return result;
}

r64
get_seconds_elapsed(Platform_Ticks start, Platform_Ticks end)
{
  Platform_Ticks diff = get_ticks_elapsed(start, end);
  return platform_ticks_to_seconds(diff);
}

// TODO(PS): we have some stuff in v1 around system time, probably
// for timestamps etc.

///////////////////////////////////////
//    Threads

struct Platform_Thread_Handle
{
  u64 value;
};

struct Platform_Thread_Result
{
  u32 code;
};

typedef struct Platform_Thread_Data Platform_Thread_Data;
typedef Platform_Thread_Result Platform_Thread_Proc(Platform_Thread_Data* thread_data);

struct Platform_Thread_Data
{
  Platform_Thread_Handle thread_handle;
  u32 thread_id;
  Platform_Thread_Proc* thread_proc;
  Allocator* thread_memory;
  u8* user_data;
};

Platform_Thread_Handle platform_thread_begin(Platform_Thread_Proc* proc, u8* user_data);
void                   platform_thread_end(Platform_Thread_Handle thread_handle);

u32 platform_interlocked_increment(volatile u32* value);
u32 platform_interlocked_cmp_exchg(volatile u32* dest, u32 new_value, u32 old_value);

///////////////////////////////////////
//    Network Access

// TODO(PS): 

struct Platform_Socket_Handle
{
  u64 value;
};

Platform_Socket_Handle platform_socket_create();
bool                   platform_socket_bind();
bool                   platform_socket_connect();
bool                   platform_socket_close();
Data                   platform_socket_recv();
s32                    platform_Socket_set_listening();
s32                    platform_Socket_send();
s32                    platform_Socket_send_to();
s32                    platform_Socket_set_opt();

///////////////////////////////////////
//    Graphics Integration

#define PLATFORM_SHADER_MAX_ATTRS 8
#define PLATFORM_SHADER_ATTR_LAST (u32)(1 << 31)
struct Platform_Shader 
{ 
  u32 id; 
  u32 attrs[PLATFORM_SHADER_MAX_ATTRS];
};

struct Platform_Geometry_Buffer 
{
  u32 buffer_id_vao;
  u32 buffer_id_vertices;
  u32 buffer_id_indices;
  u32 indices_len;
};

struct Platform_Graphics_Frame_Desc
{
  v4 clear_color;
  v2 viewport_min;
  v2 viewport_max;
};

void platform_frame_begin(Platform_Graphics_Frame_Desc desc);
void platform_frame_clear();

Platform_Geometry_Buffer platform_geometry_buffer_create(r32* vertices, u32 vertices_len, u32* indices, u32 indices_len);
Platform_Shader platform_shader_create(
                                       String code_vert, String code_frag, String* attribs, u32 attribs_len
                                       );
void platform_vertex_attrib_pointer(
                                    Platform_Geometry_Buffer geo, Platform_Shader shader, u32 attrib_index
                                    );

void platform_geometry_bind(Platform_Geometry_Buffer geo);
void platform_shader_bind(Platform_Shader shader);
void platform_geometry_draw(Platform_Geometry_Buffer geo);
void platform_vertex_attrib_pointer(
                                    Platform_Geometry_Buffer geo, Platform_Shader shader, u32 attr_index
                                    );
#endif //LUMENARIUM_PLATFORM_H
