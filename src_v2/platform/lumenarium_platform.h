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

// For Cross Platform File Operations use these:

typedef u32 Platform_File_Async_Job_Flags;
enum 
{
  PlatformFileAsyncJob_Invalid = 0,
  PlatformFileAsyncJob_Read = 1,
  PlatformFileAsyncJob_Write = 2,
  PlatformFileAsyncJob_InFlight = 4,
  PlatformFileAsyncJob_Success = 8,
  PlatformFileAsyncJob_Failed = 16,
};

struct Platform_File_Async_Job_Args
{
  String path;
  Data data;
  Platform_File_Async_Job_Flags flags;
  u32 error;
};

typedef void Platform_File_Async_Cb(Platform_File_Async_Job_Args args, u8* user_data);

struct Platform_File_Async_Job
{
  Data job_memory;
  Platform_File_Async_Job_Args args;
  Platform_File_Async_Cb* cb;
};

global Allocator*              platform_file_jobs_arena = 0;
#define PLATFORM_FILE_ASYNC_MAX_JOBS 32
global Platform_File_Async_Job platform_file_async_jobs[PLATFORM_FILE_ASYNC_MAX_JOBS];
global u32                     platform_file_async_jobs_len = 0;

void
platform_file_jobs_init()
{
  platform_file_jobs_arena = paged_allocator_create_reserve(MB(4), 256);
}

bool
platform_file_async_job_add(Platform_File_Async_Job job)
{
  if (platform_file_async_jobs_len >= PLATFORM_FILE_ASYNC_MAX_JOBS) return false;
  
  // Copy data to job local memory
  u64 size_needed = job.args.path.len + job.args.data.size + 1;
  u8* job_mem = allocator_alloc(platform_file_jobs_arena, size_needed);
  String job_path = string_create(job_mem, 0, job.args.path.len + 1);
  u64 copied = string_copy_to(&job_path, job.args.path);
  Data job_data = data_create(job_mem + job_path.cap + 1, size_needed - (job_path.cap + 1));
  memory_copy(job.args.data.base, job_data.base, job.args.data.size);
  job.args.path = job_path;
  job.args.data = job_data;
  job.job_memory = data_create(job_mem, size_needed);
  
  platform_file_async_jobs[platform_file_async_jobs_len++] = job;
  return true;
}

Platform_File_Async_Job
platform_file_async_job_rem(u64 index)
{
  assert(index < platform_file_async_jobs_len);
  Platform_File_Async_Job result = platform_file_async_jobs[index];
  
  platform_file_async_jobs_len -= 1;
  if (platform_file_async_jobs_len > 0)
  {
    u32 last_job = platform_file_async_jobs_len;
    platform_file_async_jobs[index] = platform_file_async_jobs[last_job];
  }
  
  return result;
}

bool
platform_file_async_read(String path, Platform_File_Async_Cb* cb)
{
  Platform_File_Async_Job job = {};
  job.args.path = path;
  job.args.flags = (
                    PlatformFileAsyncJob_Read |
                    PlatformFileAsyncJob_InFlight
                    );
  job.cb = cb;
  bool result = platform_file_async_job_add(job);
  return result;
}

bool
platform_file_async_write(String path, Data data, Platform_File_Async_Cb* cb)
{
  Platform_File_Async_Job job = {};
  job.args.path = path;
  job.args.data = data;
  job.args.flags = (
                    PlatformFileAsyncJob_Write |
                    PlatformFileAsyncJob_InFlight
                    );
  job.cb = cb;
  bool result = platform_file_async_job_add(job);
  return result;
}

void
platform_file_async_job_complete(Platform_File_Async_Job* job, u8* user_data)
{
  job->cb(job->args, user_data);
  allocator_free(platform_file_jobs_arena, job->job_memory.base, job->job_memory.size); 
  if (has_flag(job->args.flags, PlatformFileAsyncJob_Write))
  {
    allocator_free(platform_file_jobs_arena, job->args.data.base, job->args.data.size);
  }
}

void platform_file_async_work_on_job(Platform_File_Async_Job* job);

void
platform_file_async_jobs_do_work(u64 max_jobs, u8* user_data)
{
  u64 to_do = max_jobs;
  if (max_jobs > platform_file_async_jobs_len) to_do = platform_file_async_jobs_len;
  
  Platform_File_Async_Job_Flags completed = (
                                             PlatformFileAsyncJob_Success | 
                                             PlatformFileAsyncJob_Failed
                                             );
  
  for (u64 i = to_do - 1; i < to_do; i--)
  {
    Platform_File_Async_Job* job = platform_file_async_jobs + i;
    platform_file_async_work_on_job(job);
    if (has_flag(job->args.flags, completed))
    {
      platform_file_async_job_complete(job, user_data);
      platform_file_async_job_rem(i);
    }
  }
}

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
  WindowEvent_MouseMoved,
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
  u32 uniforms[PLATFORM_SHADER_MAX_ATTRS];
};

struct Platform_Geometry_Buffer 
{
  u32 buffer_id_vao;
  u32 buffer_id_vertices;
  u32 buffer_id_indices;
  u32 vertices_len;
  u32 indices_len;
};

struct Platform_Texture
{
  u32 id;
  
  u32 w, h, s;
};

struct Platform_Graphics_Frame_Desc
{
  v4 clear_color;
  v2 viewport_min;
  v2 viewport_max;
};

void platform_frame_begin(Platform_Graphics_Frame_Desc desc);
void platform_frame_clear();

// Geometry
Platform_Geometry_Buffer platform_geometry_buffer_create(r32* vertices, u32 vertices_len, u32* indices, u32 indices_len);
Platform_Shader platform_shader_create(
                                       String code_vert, String code_frag, String* attribs, u32 attribs_len, String* uniforms, u32 uniforms_len
                                       );
void platform_geometry_buffer_update(Platform_Geometry_Buffer* buffer, r32* verts, u32 verts_offset, u32 verts_len, u32* indices, u32 indices_offset, u32 indices_len);

// Shaders
void platform_geometry_bind(Platform_Geometry_Buffer geo);
void platform_shader_bind(Platform_Shader shader);
void platform_geometry_draw(Platform_Geometry_Buffer geo, u32 indices);
void platform_geometry_draw(Platform_Geometry_Buffer geo);
void platform_vertex_attrib_pointer(
                                    Platform_Geometry_Buffer geo, Platform_Shader shader, u32 count, u32 attr_index, u32 stride, u32 offset
                                    );
void platform_set_uniform(Platform_Shader shader, u32 index, m44 u);

// Textures
Platform_Texture platform_texture_create(u8* pixels, u32 width, u32 height, u32 stride);
void platform_texture_bind(Platform_Texture tex);
void platform_texture_update(Platform_Texture tex, u8* new_pixels, u32 width, u32 height, u32 stride);

#endif //LUMENARIUM_PLATFORM_H
