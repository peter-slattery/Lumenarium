#include "gs_language.h"
#include "gs_platform.h"

#include "foldhaus_memory.h"
#include "gs_string.h"
#include "gs_input.h"
#include "foldhaus_debug.h"

global_variable debug_services* GlobalDebugServices;

#include "gs_vector_matrix.h"
#include "foldhaus_renderer.h"

typedef struct context context;


// Application Functions

#define INITIALIZE_APPLICATION(name) void name(context Context)
typedef INITIALIZE_APPLICATION(initialize_application);

#define UPDATE_AND_RENDER(name) void name(context Context, input Input, render_command_buffer* RenderBuffer)
typedef UPDATE_AND_RENDER(update_and_render);

#define RELOAD_STATIC_DATA(name) void name(context Context, debug_services* DebugServices)
typedef RELOAD_STATIC_DATA(reload_static_data);

#define CLEANUP_APPLICATION(name) void name(context Context)
typedef CLEANUP_APPLICATION(cleanup_application);

// Platform Functions

// Worker Threads

#define THREADED_WORK_PROC(name) void name(s32 ThreadID, void* Data)
typedef THREADED_WORK_PROC(threaded_work_proc);

typedef struct work_queue work_queue;

#define PUSH_WORK_ON_QUEUE(name) void name(work_queue* Queue, threaded_work_proc* WorkProc, void* Data)
typedef PUSH_WORK_ON_QUEUE(push_work_on_queue);

#define DO_QUEUE_WORK_UNTIL_DONE(name) void name(work_queue* Queue, s32 ThreadID)
typedef DO_QUEUE_WORK_UNTIL_DONE(do_queue_work_until_done);

#define RESET_WORK_QUEUE(name) void name(work_queue* Queue)
typedef RESET_WORK_QUEUE(reset_work_queue);

struct worker_thread_job
{
    void* Data;
    threaded_work_proc* WorkProc;
};

struct work_queue
{
    HANDLE SemaphoreHandle;
    
    u32 JobsMax;
    u32 volatile JobsCount;
    u32 volatile NextJobIndex;
    u32 volatile JobsCompleted;
    worker_thread_job Jobs[256];
    
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

struct context
{
    u8* MemoryBase;
    u32 MemorySize;
    
    b32 WindowIsVisible;
    r32 WindowWidth;
    r32 WindowHeight;
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
    platform_read_entire_file* PlatformReadEntireFile;
    platform_write_entire_file* PlatformWriteEntireFile;
    platform_get_file_path* PlatformGetFilePath;
    platform_get_gpu_texture_handle* PlatformGetGPUTextureHandle;
    
    platform_get_socket_handle* PlatformGetSocketHandle;
    platform_get_send_address* PlatformGetSendAddress;
    platform_set_socket_option* PlatformSetSocketOption;
    platform_send_to* PlatformSendTo;
    platform_close_socket* PlatformCloseSocket;
};
