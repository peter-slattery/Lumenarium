//
// File: foldhaus_platform.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PLATFORM_H

// TODO Remove
#include <math.h>
#include <windows.h>

#include "..\gs_libs\gs_types.h"
#include "..\gs_libs\gs_types.cpp"
#include "..\gs_libs\gs_path.h"

struct handle
{
  u32 Generation;
  u32 Index;
};

inline bool
Handle_IsValid(handle Handle)
{
  bool Result = (Handle.Generation != 0);
  return Result;
}

#include "..\gs_libs\gs_string.h"
#include "..\gs_libs\gs_csv.h"

#include "engine/foldhaus_log.h"
global log_buffer* GlobalLogBuffer;

#include "foldhaus_debug.h"
global debug_services* GlobalDebugServices;

//#include "..\gs_libs\gs_vector_matrix.h"
#include "..\gs_libs\gs_input.h"

struct platform_network_address
{
  s32 Family;
  u16 Port;
  u32 Address;
};

typedef s32 platform_socket_handle;
typedef s32 platform_network_address_handle;

#include "foldhaus_renderer.h"
#include "engine/foldhaus_addressed_data.h"

typedef struct context context;

// Application Functions

// TODO(pjs): TEMP
typedef void temp_job_req_proc(gs_thread_context* Ctx, u8* Memory);
struct temp_job_req
{
  temp_job_req_proc* Proc;
  u8* Memory;
};
// This isn't necessarily temp but I'm not sure it goes here
#define PACKETS_MAX 32
struct packet_ringbuffer
{
  gs_data Values[PACKETS_MAX];
  u32 ReadHead;
  u32 WriteHead;
};

#define INITIALIZE_APPLICATION(name) void name(context* Context)
typedef INITIALIZE_APPLICATION(initialize_application);

#define UPDATE_AND_RENDER(name) void name(context* Context, input_queue InputQueue, render_command_buffer* RenderBuffer, addressed_data_buffer_list* OutputData)
typedef UPDATE_AND_RENDER(update_and_render);

#define RELOAD_STATIC_DATA(name) void name(context Context, debug_services* DebugServices, log_buffer* LogBuffer, bool AppReady)
typedef RELOAD_STATIC_DATA(reload_static_data);

#define CLEANUP_APPLICATION(name) void name(context Context, addressed_data_buffer_list* OutputData)
typedef CLEANUP_APPLICATION(cleanup_application);

// Platform Functions

struct window_info
{
  char* Name;
  s32 Width;
  s32 Height;
};

typedef struct window window;

#define PLATFORM_MEMORY_NO_ERROR 0
enum platform_memory_error
{
  PlatformMemory_NoError,
  PlatformMemory_FileNotFound,
  
  PlatformMemory_UnknownError, // You should implement handling this when you see it
};

struct data
{
  u8* Base;
  u64 Size;
};

struct platform_memory_result
{
  data Data;
  platform_memory_error Error;
};

struct system_path
{
  char* Path;
  s32 PathLength;
  s32 IndexOfLastSlash;
};

struct texture_buffer
{
  u8* Memory;
  s32 Width;
  s32 Height;
  s32 Pitch;
  s32 BytesPerPixel;
};

#define PLATFORM_GET_GPU_TEXTURE_HANDLE(name) s32 name(u8* Memory, s32 Width, s32 Height)
typedef PLATFORM_GET_GPU_TEXTURE_HANDLE(platform_get_gpu_texture_handle);

#define PLATFORM_GET_SOCKET_HANDLE(name) platform_socket_handle name(s32 Multicast_TimeToLive)
typedef PLATFORM_GET_SOCKET_HANDLE(platform_get_socket_handle);

// Font
struct platform_font_info
{
  s32 PixelHeight;
  s32 Ascent, Descent, Leading;
  s32 MaxCharWidth;
  s32 CodepointStart;
  s32 CodepointOnePastLast;
};

enum font_weight
{
  FontWeight_Invalid = 0,
  FontWeight_Thin = 100,
  FontWeight_ExtraLight = 200,
  FontWeight_Light = 300,
  FontWeight_Normal = 400,
  FontWeight_Medium = 500,
  FontWeight_SemiBold = 600,
  FontWeight_Bold = 700,
  FontWeight_ExtraBold = 800,
  FontWeight_Heavy = 900,
};

#define GET_FONT_INFO(name) platform_font_info name(char* FontName, s32 PixelHeight, font_weight FontWeight, b32 Italic, b32 Underline, b32 Strikeout)
typedef GET_FONT_INFO(platform_get_font_info);

#define DRAW_FONT_CODEPOINT(name) void name(u8* DestBuffer, s32 DestBufferWidth, s32 DestBufferHeight, u32 XOffset, u32 YOffset, char Codepoint, platform_font_info FontInfo, u32* OutWidth, u32* OutHeight)
typedef DRAW_FONT_CODEPOINT(platform_draw_font_codepoint);

// Worker Threads

#define PLATFORM_THREAD_COUNT 3

RESET_WORK_QUEUE(ResetWorkQueue)
{
  for (u32 i = 0; i < Queue->JobsMax; i++)
  {
    Queue->Jobs[i].Data = {0};
    Queue->Jobs[i].WorkProc = 0;
  }
  
  Queue->JobsCount = 0;
  Queue->NextJobIndex = 0;
  Queue->JobsCompleted = 0;
}

// Time

internal r32
GetSecondsElapsed (s64 Start, s64 End, s64 PerformanceCountFrequency)
{
  r32 Result = ((r32)(End - Start) / (r32) PerformanceCountFrequency);
  return Result;
}

typedef struct system_time
{
  u64 NanosSinceEpoch;
  
  s32 Year;
  s32 Month;
  s32 Day;
  s32 Hour; // [0:23]
  s32 Minute;
  s32 Second;
} system_time;

internal r64
SecondsElapsed(system_time Start, system_time End)
{
  u64 N = End.NanosSinceEpoch - Start.NanosSinceEpoch;
  r64 S = (r64)N * NanosToSeconds;
  return S;
}

struct context
{
  gs_thread_context ThreadContext;
  
  u8* MemoryBase;
  u32 MemorySize;
  
  b32 WindowIsVisible;
  rect2 WindowBounds;
  r64 TotalTime;
  r32 DeltaTime;
  mouse_state Mouse;
  
  // Application Services
  initialize_application* InitializeApplication;
  reload_static_data* ReloadStaticData;
  update_and_render* UpdateAndRender;
  cleanup_application* CleanupApplication;
  
  platform_thread_manager* ThreadManager;
  platform_socket_manager* SocketManager;
  
  // Platform Services
  gs_work_queue* GeneralWorkQueue;
  
  platform_get_gpu_texture_handle* PlatformGetGPUTextureHandle;
  platform_get_font_info* PlatformGetFontInfo;
  platform_draw_font_codepoint* PlatformDrawFontCodepoint;
  
  platform_get_socket_handle* PlatformGetSocketHandle;
  
  system_time SystemTime_Last;
  system_time SystemTime_Current;
  
  // 
  bool Headless;
};

#define FOLDHAUS_PLATFORM_H
#endif // FOLDHAUS_PLATFORM_H