//
// File: foldhaus_platform.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PLATFORM_H

#include <windows.h>

#define GS_LANGUAGE_NO_PROFILER_DEFINES
#include "..\gs_libs\gs_language.h"

#include "..\gs_libs\gs_radix_sort.h"
#include "..\gs_libs\gs_list.h"
#include "..\gs_libs\gs_bucket.h"

#define GS_MEMORY_TRACK_ALLOCATIONS
#include "..\gs_libs\gs_memory_arena.h"

#include "..\gs_libs\gs_string.h"

#include "foldhaus_debug.h"
global_variable debug_services* GlobalDebugServices;

#include "..\gs_libs\gs_vector_matrix.h"
#include "..\gs_libs\gs_input.h"

#include "foldhaus_renderer.h"

typedef struct context context;

// Application Functions

#define INITIALIZE_APPLICATION(name) void name(context Context)
typedef INITIALIZE_APPLICATION(initialize_application);

#define UPDATE_AND_RENDER(name) void name(context Context, input_queue InputQueue, mouse_state Mouse, render_command_buffer* RenderBuffer)
typedef UPDATE_AND_RENDER(update_and_render);

#define RELOAD_STATIC_DATA(name) void name(context Context, debug_services* DebugServices)
typedef RELOAD_STATIC_DATA(reload_static_data);

#define CLEANUP_APPLICATION(name) void name(context Context)
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

struct platform_memory_result
{
    u8* Base;
    s32 Size;
    platform_memory_error Error;
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

#define PLATFORM_ALLOC(name) u8* name(s32 Size)
typedef PLATFORM_ALLOC(platform_alloc);

#define PLATFORM_FREE(name) b32 name(u8* Base, s32 Size)
typedef PLATFORM_FREE(platform_free);

#define PLATFORM_REALLOC(name) u8* name(u8* Base, u32 OldSize, u32 NewSize)
typedef PLATFORM_REALLOC(platform_realloc);

#define PLATFORM_READ_ENTIRE_FILE(name) platform_memory_result name(char* Path)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char* Path, u8* Contents, s32 Size)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_GET_FILE_PATH(name) b32 name(char* PathBuffer, s32 BufferLength, const char* FilterStrings)
typedef PLATFORM_GET_FILE_PATH(platform_get_file_path);

#define PLATFORM_GET_GPU_TEXTURE_HANDLE(name) s32 name(u8* Memory, s32 Width, s32 Height)
typedef PLATFORM_GET_GPU_TEXTURE_HANDLE(platform_get_gpu_texture_handle);

struct platform_network_address
{
    s32 Family;
    u16 Port;
    u32 Address;
};

typedef s32 platform_socket_handle;
typedef s32 platform_network_address_handle;

#define PLATFORM_GET_SOCKET_HANDLE(name) platform_socket_handle name(s32 Multicast_TimeToLive) 
typedef PLATFORM_GET_SOCKET_HANDLE(platform_get_socket_handle);

#define PLATFORM_GET_SEND_ADDRESS_HANDLE(name) platform_network_address_handle name(s32 AddressFamily, u16 Port, u32 Address)
typedef PLATFORM_GET_SEND_ADDRESS_HANDLE(platform_get_send_address);

#define PLATFORM_SET_SOCKET_OPTION(name) s32 name(platform_socket_handle SocketHandle, s32 Level, s32 Option, const char* OptionValue, s32 OptionLength) 
typedef PLATFORM_SET_SOCKET_OPTION(platform_set_socket_option);

#define PLATFORM_SEND_TO(name) s32 name(platform_socket_handle SocketHandle, u32 Address, u32 Port, const char* Buffer, s32 BufferLength, s32 Flags)
typedef PLATFORM_SEND_TO(platform_send_to);

#define PLATFORM_CLOSE_SOCKET(name) void name(platform_socket_handle SocketHandle)
typedef PLATFORM_CLOSE_SOCKET(platform_close_socket);

// File IO

// TODO(Peter): 
struct directory_listing
{
    string Path;
    directory_listing* Next;
};

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

#define PLATFORM_THREAD_COUNT 4

#define THREADED_WORK_PROC(name) void name(s32 ThreadID, void* Data)
typedef THREADED_WORK_PROC(threaded_work_proc);

typedef struct work_queue work_queue;

#define PUSH_WORK_ON_QUEUE(name) void name(work_queue* Queue, threaded_work_proc* WorkProc, void* Data, char* JobName)
typedef PUSH_WORK_ON_QUEUE(push_work_on_queue);

#define DO_QUEUE_WORK_UNTIL_DONE(name) void name(work_queue* Queue, s32 ThreadID)
typedef DO_QUEUE_WORK_UNTIL_DONE(do_queue_work_until_done);

#define RESET_WORK_QUEUE(name) void name(work_queue* Queue)
typedef RESET_WORK_QUEUE(reset_work_queue);

struct worker_thread_job
{
    void* Data;
    threaded_work_proc* WorkProc;
#ifdef DEBUG
    char* JobName;
#endif
};

struct work_queue
{
    void* SemaphoreHandle;
    
    u32 JobsMax;
    u32 volatile JobsCount;
    u32 volatile NextJobIndex;
    u32 volatile JobsCompleted;
    worker_thread_job* Jobs;
    
    // Work Queue
    push_work_on_queue* PushWorkOnQueue;
    do_queue_work_until_done* DoQueueWorkUntilDone;
    reset_work_queue* ResetWorkQueue;
};

RESET_WORK_QUEUE(ResetWorkQueue)
{
    for (u32 i = 0; i < Queue->JobsMax; i++)
    {
        Queue->Jobs[i].Data = 0;
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

struct context
{
    u8* MemoryBase;
    u32 MemorySize;
    
    b32 WindowIsVisible;
    rect WindowBounds;
    r32 DeltaTime;
    
    // Application Services
    initialize_application* InitializeApplication;
    reload_static_data* ReloadStaticData;
    update_and_render* UpdateAndRender;
    cleanup_application* CleanupApplication;
    
    // Platform Services
    work_queue* GeneralWorkQueue;
    
    platform_alloc* PlatformAlloc;
    platform_free* PlatformFree;
    platform_realloc* PlatformRealloc;
    platform_read_entire_file* PlatformReadEntireFile;
    platform_write_entire_file* PlatformWriteEntireFile;
    platform_get_file_path* PlatformGetFilePath;
    platform_get_gpu_texture_handle* PlatformGetGPUTextureHandle;
    platform_get_font_info* PlatformGetFontInfo;
    platform_draw_font_codepoint* PlatformDrawFontCodepoint;
    platform_get_socket_handle* PlatformGetSocketHandle;
    platform_set_socket_option* PlatformSetSocketOption;
    platform_send_to* PlatformSendTo;
    platform_close_socket* PlatformCloseSocket;
};


#define FOLDHAUS_PLATFORM_H
#endif // FOLDHAUS_PLATFORM_H