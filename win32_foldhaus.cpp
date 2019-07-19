#include <Winsock2.h>
#include <ws2tcpip.h>
#include <intrin.h>

#include "foldhaus_platform.h"

#include "gs_win32.h"

#include "gs_opengl.h"
#include "foldhaus_renderer.cpp"

global_variable b32 Running = false;
global_variable b32 WindowIsActive = false;

char DLLName[] = "foldhaus.dll";
char WorkingDLLName[] = "foldhaus_temp.dll";
char DLLLockFileName[] = "lock.tmp";

win32_window MainWindow;

struct worker_thread_entry
{
    b32 IsValid;
    u32 Index;
};

struct worker_thread_info
{
    s32 ID;
    HANDLE Handle;
    work_queue* Queue;
};

PUSH_WORK_ON_QUEUE(Win32PushWorkOnQueue)
{
    Assert(Queue->JobsCount < Queue->JobsMax);
    
    worker_thread_job* Job = Queue->Jobs + Queue->JobsCount;
    Job->WorkProc = WorkProc;
    Job->Data = Data;
    
    // Complete Past Writes before Future Writes
    _WriteBarrier();
    _mm_sfence();
    
    ++Queue->JobsCount;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal worker_thread_entry
CompleteAndTakeNextJob(work_queue* Queue, worker_thread_entry Completed)
{
    if (Completed.IsValid)
    {
        InterlockedIncrement((LONG volatile*)&Queue->JobsCompleted);
    }
    
    worker_thread_entry Result = {};
    Result.IsValid = false;
    
    u32 OriginalNextJobIndex = Queue->NextJobIndex;
    while (OriginalNextJobIndex < Queue->JobsCount)
    {
        u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextJobIndex,
                                               OriginalNextJobIndex + 1,
                                               OriginalNextJobIndex);
        if (Index == OriginalNextJobIndex)
        {
            Result.Index = Index;
            Result.IsValid = true;
            break;
        }
        OriginalNextJobIndex = Queue->NextJobIndex;
    }
    
    return Result;
}

DO_QUEUE_WORK_UNTIL_DONE(Win32DoQueueWorkUntilDone)
{
    worker_thread_entry Entry = {};
    Entry.IsValid = false;
    while (Queue->JobsCompleted < Queue->JobsCount)
    {
        Entry = CompleteAndTakeNextJob(Queue, Entry);
        if (Entry.IsValid)
        {
            Queue->Jobs[Entry.Index].WorkProc(ThreadID, Queue->Jobs[Entry.Index].Data);
        }
    }
}

DWORD WINAPI
WorkerThreadProc (LPVOID InputThreadInfo)
{
    worker_thread_info* ThreadInfo = (worker_thread_info*)InputThreadInfo;
    
    worker_thread_entry Entry = {};
    Entry.IsValid = false;
    while (true)
    {
        Entry = CompleteAndTakeNextJob(ThreadInfo->Queue, Entry);
        if (Entry.IsValid)
        {
            ThreadInfo->Queue->Jobs[Entry.Index].WorkProc(ThreadInfo->ID, 
                                                          ThreadInfo->Queue->Jobs[Entry.Index].Data);
        }
        else
        {
            WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, 0);
        }
    }
    
    return 0;
}

PLATFORM_GET_GPU_TEXTURE_HANDLE(Win32GetGPUTextureHandle)
{
    s32 Handle = SubmitTexture(Width, Height, Memory);
    return Handle;
}

struct win32_socket
{
    SOCKET Socket;
};

#define SOCKET_DICTIONARY_GROW_SIZE 32
s32 Win32SocketHandleMax;
s32 Win32SocketHandleCount;
win32_socket* SocketValues;

PLATFORM_GET_SOCKET_HANDLE(Win32GetSocketHandle)
{
    if (Win32SocketHandleCount >= Win32SocketHandleMax)
    {
        s32 NewDictionaryMax = Win32SocketHandleMax + SOCKET_DICTIONARY_GROW_SIZE;
        s32 NewDictionaryDataSize = NewDictionaryMax *  sizeof(win32_socket);
        platform_memory_result DictionaryMemory = Win32Alloc(NewDictionaryDataSize);
        Assert(DictionaryMemory.Size > 0);
        
        win32_socket* NewValues = (win32_socket*)(DictionaryMemory.Base);
        if (SocketValues)
        {
            GSMemCopy(SocketValues, NewValues, sizeof(win32_socket) * NewDictionaryMax);
            Win32Free((u8*)SocketValues, sizeof(win32_socket) * Win32SocketHandleCount);
        }
        SocketValues = NewValues;
        
        Win32SocketHandleMax = NewDictionaryMax;
    }
    
    Assert(Win32SocketHandleCount < Win32SocketHandleMax);
    s32 NewSocketIndex = Win32SocketHandleCount++;
    
    SocketValues[NewSocketIndex].Socket = socket(AddressFamily, Type, Protocol);
    
    return (platform_socket_handle)NewSocketIndex;
}

#define NETWORK_ADDRESS_DICTIONARY_GROW_SIZE 32
s32 Win32NetworkAddressHandleMax;
s32 Win32NetworkAddressHandleCount;
sockaddr_in* NetworkAddressValues;

PLATFORM_GET_SEND_ADDRESS_HANDLE(Win32GetSendAddress)
{
    if (Win32NetworkAddressHandleCount >= Win32NetworkAddressHandleMax)
    {
        s32 NewDictionaryMax = Win32NetworkAddressHandleMax + NETWORK_ADDRESS_DICTIONARY_GROW_SIZE;
        s32 NewDictionaryDataSize = NewDictionaryMax *  sizeof(sockaddr_in);
        platform_memory_result DictionaryMemory = Win32Alloc(NewDictionaryDataSize);
        Assert(DictionaryMemory.Size > 0);
        
        sockaddr_in* NewValues = (sockaddr_in*)(DictionaryMemory.Base);
        if (NetworkAddressValues)
        {
            GSMemCopy(NetworkAddressValues, NewValues, sizeof(win32_socket) * NewDictionaryMax);
            Win32Free((u8*)NetworkAddressValues, sizeof(win32_socket) * Win32NetworkAddressHandleCount);
        }
        NetworkAddressValues = NewValues;
        
        Win32NetworkAddressHandleMax = NewDictionaryMax;
    }
    
    Assert(Win32NetworkAddressHandleCount < Win32NetworkAddressHandleMax);
    s32 NewAddressIndex = Win32NetworkAddressHandleCount++;
    
    NetworkAddressValues[NewAddressIndex].sin_family = Family;
    NetworkAddressValues[NewAddressIndex].sin_port = HostToNetU16(Port);
    NetworkAddressValues[NewAddressIndex].sin_addr.s_addr = HostToNetU32(Address);
    
    return (platform_network_address_handle)NewAddressIndex;
}

PLATFORM_SET_SOCKET_OPTION(Win32SetSocketOption)
{
    s32 SocketIndex = (s32)SocketHandle;
    Assert(SocketIndex < Win32SocketHandleCount);
    int Error = setsockopt(SocketValues[SocketIndex].Socket, Level, Option, OptionValue, OptionLength);
    if (Error == SOCKET_ERROR)
    {
        Error = WSAGetLastError();
    }
    
    return Error;
}

PLATFORM_SEND_TO(Win32SendTo)
{
    s32 SocketIndex = (s32)SocketHandle;
    Assert(SocketIndex < Win32SocketHandleCount);
    
    s32 AddressIndex = (s32)AddressHandle;
    Assert(AddressIndex < Win32NetworkAddressHandleCount);
    
    s32 LengthSent = sendto(SocketValues[SocketIndex].Socket, Buffer, BufferLength, Flags, (sockaddr*)&NetworkAddressValues[AddressIndex], sizeof(sockaddr_in));
    
    if (LengthSent == SOCKET_ERROR)
    {
        s32 Error = WSAGetLastError();
        InvalidCodePath;
    }
    
    return LengthSent;
}

PLATFORM_CLOSE_SOCKET(Win32CloseSocket)
{
    s32 SocketIndex = (s32)SocketHandle;
    Assert(SocketIndex < Win32SocketHandleCount);
    
    closesocket(SocketValues[SocketIndex].Socket);
}

LRESULT CALLBACK
HandleWindowEvents (HWND WindowHandle, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch (Msg)
    {
        case WM_SIZE:
        {
            Win32UpdateWindowDimension(&MainWindow);
            //Win32ResizeDIBSection(&GlobalBackbuffer, MainWindow.Info.Width, MainWindow.Info.Height);
        }break;
        
        case WM_CLOSE:
        {
            Result = DefWindowProc(WindowHandle, Msg, WParam, LParam);
            Running = false;
        }break;
        
        case WM_DESTROY:
        {
        }break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            HDC DeviceContext;
            b32 PaintResult;
            
            DeviceContext = BeginPaint(WindowHandle, &PaintStruct);
            PaintResult = EndPaint(WindowHandle, &PaintStruct);
        }break;
        
        case WM_ACTIVATE:
        {
            WindowIsActive = (LOWORD(WParam) == WA_ACTIVE || LOWORD(WParam) == WA_CLICKACTIVE);
        }break;
        
        default:
        {
            Result = DefWindowProc(WindowHandle, Msg, WParam, LParam);
        }
    }
    
    return Result;
}

internal void
HandleWindowMessage (MSG Message, win32_window* Window, input_frame* InputFrame)
{
    switch (Message.message)
    {
        case WM_MOUSEWHEEL:
        {
            int MouseWheel = GET_WHEEL_DELTA_WPARAM(Message.wParam);
            InputFrame->MouseScroll = MouseWheel;
        }break;
        
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            InputFrame->KeysDown[KeyCode_MouseLeftButton]   = (GetKeyState(VK_LBUTTON) & (1 << 15)) != 0;
            InputFrame->KeysDown[KeyCode_MouseMiddleButton] = (GetKeyState(VK_MBUTTON) & (1 << 15)) != 0;
            InputFrame->KeysDown[KeyCode_MouseRightButton]  = (GetKeyState(VK_RBUTTON) & (1 << 15)) != 0;
            // NOTE(Peter): If you decide to support extra mouse buttons, on windows the key codes are
            // VK_XBUTTON1 and VK_XBUTTON2
        }break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            int VirtualKey = (int)Message.wParam;
            bool KeyDown = (Message.lParam & (1 << 31)) == 0;
            int KeyIndex = Win32GetKeyIndex(VirtualKey, true, false);
            if (KeyIndex == WIN32_SHOULD_TRANSLATE_TO_CHAR)
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
            else
            {
                InputFrame->KeysDown[KeyIndex] = KeyDown;
            }
        }break;
        
        case WM_CHAR:
        {
            char Char = (char)Message.wParam;
            InputFrame->StringInput[InputFrame->StringInputUsed++] = Char;
        }break;
        
        default:
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }break;
    }
}

internal void
DebugPrint (char* Format, ...)
{
    char Buffer[256];
    va_list Args;
    va_start(Args, Format);
    PrintFInternal(Buffer, 256, Format, Args);
    OutputDebugStringA(Buffer);
    va_end(Args);
}

internal void
SetApplicationLinks (context* Context, win32_dll_refresh DLL, work_queue* WorkQueue)
{
    if (DLL.IsValid)
    {
        Context->InitializeApplication = (initialize_application*)GetProcAddress(DLL.DLL, "InitializeApplication");
        Context->ReloadStaticData = (reload_static_data*)GetProcAddress(DLL.DLL, "ReloadStaticData");
        Context->UpdateAndRender = (update_and_render*)GetProcAddress(DLL.DLL, "UpdateAndRender");
        Context->CleanupApplication = (cleanup_application*)GetProcAddress(DLL.DLL, "CleanupApplication");
    }
    else
    {
        Context->InitializeApplication = 0;
        Context->ReloadStaticData = 0;
        Context->UpdateAndRender = 0;
        Context->CleanupApplication = 0;
    }
}

int WINAPI
WinMain (
HINSTANCE HInstance,
HINSTANCE HPrevInstance,
PSTR CmdLineArgs,
INT NCmdShow
)
{
    win32_window_info MainWindowInfo = {};
    MainWindowInfo.Name = "Foldhaus";
    MainWindowInfo.ClassName = "Foldhaus Window Class";
    MainWindowInfo.Width = 1440;
    MainWindowInfo.Height = 768;
    MainWindowInfo.WindowEventsHandler = HandleWindowEvents;
    
    MainWindow = CreateWin32Window (HInstance, MainWindowInfo);
    Win32UpdateWindowDimension(&MainWindow);
    
    win32_opengl_window_info OpenGLWindowInfo = {};
    OpenGLWindowInfo.ColorBits = 32;
    OpenGLWindowInfo.AlphaBits = 8;
    OpenGLWindowInfo.DepthBits = 0;
    CreateOpenGLWindowContext(OpenGLWindowInfo, &MainWindow);
    
    s64 PerformanceCountFrequency = GetPerformanceFrequency();
    GlobalDebugServices = (debug_services*)malloc(sizeof(debug_services));
    InitDebugServices(GlobalDebugServices, (u8*)malloc(Megabytes(8)), Megabytes(8), 1000, PerformanceCountFrequency);
    GlobalDebugServices->GetWallClock = GetWallClock;
    
    input Input;
    InitializeInput(&Input);
    
    //
    // Set up worker threads
    //
    
    const s32 WorkerThreadCount = 2;
    worker_thread_info* WorkerThreads = 0;
    if (WorkerThreadCount > 0)
    {
        WorkerThreads = (worker_thread_info*)malloc(sizeof(worker_thread_info) * WorkerThreadCount);
    }
    
    work_queue WorkQueue = {};
    WorkQueue.SemaphoreHandle = CreateSemaphoreEx(0, 0, WorkerThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    WorkQueue.JobsMax = 256;
    WorkQueue.NextJobIndex = 0;
    WorkQueue.PushWorkOnQueue = Win32PushWorkOnQueue;
    WorkQueue.DoQueueWorkUntilDone = Win32DoQueueWorkUntilDone;
    WorkQueue.ResetWorkQueue = ResetWorkQueue;
    
    for (s32 i = 0; i < WorkerThreadCount; i++)
    {
        // ID = 0 is reserved for this thread
        WorkerThreads[i].ID = i + 1;
        WorkerThreads[i].Queue = &WorkQueue;
        WorkerThreads[i].Handle = CreateThread(0, 0, &WorkerThreadProc, (void*)&WorkerThreads[i], 0, 0);
    }
    
    platform_memory_result InitialMemory = Win32Alloc(Megabytes(64));
    context Context = {};
    Context.MemorySize = InitialMemory.Size;
    Context.MemoryBase = InitialMemory.Base;
    Context.WindowWidth = MainWindow.Info.Width;
    Context.WindowHeight = MainWindow.Info.Height;
    
    // Platform functions
    Context.GeneralWorkQueue = &WorkQueue;
    Context.PlatformAlloc = Win32Alloc;
    Context.PlatformFree = Win32Free;
    Context.PlatformReadEntireFile = ReadEntireFile;
    Context.PlatformWriteEntireFile = WriteEntireFile;
    Context.PlatformGetFilePath = Win32SystemDialogOpenFile;
    Context.PlatformGetGPUTextureHandle = Win32GetGPUTextureHandle;
    Context.PlatformGetSocketHandle = Win32GetSocketHandle;
    Context.PlatformGetSendAddress = Win32GetSendAddress;
    Context.PlatformSetSocketOption = Win32SetSocketOption;
    Context.PlatformCloseSocket = Win32CloseSocket;
    
    win32_dll_refresh DLLRefresh = InitializeDLLHotReloading(DLLName, WorkingDLLName, DLLLockFileName);
    if (HotLoadDLL(&DLLRefresh))
    {
        SetApplicationLinks(&Context, DLLRefresh, &WorkQueue);
        Context.ReloadStaticData(Context, GlobalDebugServices);
    }
    else
    {
        InvalidCodePath;
    }
    
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    
    platform_memory_result RenderMemory = Win32Alloc(Megabytes(32));
    render_command_buffer RenderBuffer = AllocateRenderCommandBuffer(RenderMemory.Base, RenderMemory.Size);
    
    Context.InitializeApplication(Context);
    
    Running = true;
    Context.WindowIsVisible = true;
    while (Running)
    {
        DEBUG_TRACK_SCOPE(MainLoop);
        
        SwapInputBuffers(&Input);
        if (HotLoadDLL(&DLLRefresh))
        {
            SetApplicationLinks(&Context, DLLRefresh, &WorkQueue);
            Context.ReloadStaticData(Context, GlobalDebugServices);
        }
        
        MSG Message;
        while (PeekMessageA(&Message, MainWindow.Handle, 0, 0, PM_REMOVE))
        {
            HandleWindowMessage(Message, &MainWindow, Input.New);
        }
        
        { // Mouse Position
            POINT MousePos;
            GetCursorPos (&MousePos);
            ScreenToClient(MainWindow.Handle, &MousePos);
            Input.New->MouseX = MousePos.x;
            Input.New->MouseY = MainWindow.Info.Height - MousePos.y;
            
            if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton))
            {
                Input.MouseDownX = Input.New->MouseX;
                Input.MouseDownY = Input.New->MouseY;
            }
        }
        
        // TODO(Peter): We shouldn't need to do this translation. the platform layer knows about win32_windows. We should just make that the interface
        // to all windows.
        Context.WindowWidth = MainWindow.Info.Width;
        Context.WindowHeight = MainWindow.Info.Height;
        Context.DeltaTime = LastFrameSecondsElapsed;
        
        Context.UpdateAndRender(Context, Input, &RenderBuffer);
        
        RenderCommandBuffer(RenderBuffer);
        ClearRenderBuffer(&RenderBuffer);
        
        ///////////////////////////////////
        //        Finish Up
        //////////////////////////////////
        
        HDC DeviceContext = GetDC(MainWindow.Handle);
        SwapBuffers(DeviceContext);
        ReleaseDC(MainWindow.Handle, DeviceContext);
        
        s64 FinishedWorkTime = GetWallClock();
        r32 SecondsElapsed = GetSecondsElapsed(LastFrameEnd, FinishedWorkTime, PerformanceCountFrequency);
        
        while (SecondsElapsed < TargetSecondsPerFrame)
        {
            u32 SleepTime = 1000.0f * (TargetSecondsPerFrame - SecondsElapsed);
            Sleep(SleepTime);
            SecondsElapsed = GetSecondsElapsed(LastFrameEnd, GetWallClock(), PerformanceCountFrequency);
        }
        
        LastFrameSecondsElapsed = SecondsElapsed;
        LastFrameEnd = GetWallClock();
    }
    
    Context.CleanupApplication(Context);
    
    s32 CleanupResult = 0;
    do {
        CleanupResult = WSACleanup();
    }while(CleanupResult == SOCKET_ERROR);
    
    for (s32 Thread = 0; Thread < WorkerThreadCount; Thread++)
    {
        TerminateThread(WorkerThreads[Thread].Handle, 0);
    }
    
    return 0;
}