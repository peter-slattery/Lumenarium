//
// File: win32_foldhaus.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef WIN32_FOLDHAUS_CPP

#include <Winsock2.h>
#include <ws2tcpip.h>
#include <intrin.h>
#include <windowsx.h>
#include <gl/gl.h>

#include "../meta/gs_meta_include.h"
#include "foldhaus_platform.h"

#include "../gs_libs/gs_win32.cpp"
#include "win32_foldhaus_memory.h"
#include "win32_foldhaus_fileio.h"
#include "win32_foldhaus_dll.h"
#include "win32_foldhaus_timing.h"

#include "foldhaus_renderer.cpp"

global_variable b32 Running = false;
global_variable b32 WindowIsActive = false;

char DLLName[] = "foldhaus.dll";
char WorkingDLLName[] = "foldhaus_temp.dll";
char DLLLockFileName[] = "lock.tmp";

window MainWindow;

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
#ifdef DEBUG
    // NOTE(Peter): Just prints out the names of all the pending jobs if we end up
    // overflowing the buffer
    if (Queue->JobsCount >= Queue->JobsMax)
    {
        string DebugString = MakeString((char*)malloc(256), 256);
        for (u32 i = 0; i < Queue->JobsCount; i++)
        {
            PrintF(&DebugString, "%d %s\n", i, Queue->Jobs[i].JobName);
            NullTerminate(&DebugString);
            OutputDebugStringA(DebugString.Memory);
        }
    }
#endif
    Assert(Queue->JobsCount < Queue->JobsMax);
    
    worker_thread_job* Job = Queue->Jobs + Queue->JobsCount;
    Job->WorkProc = WorkProc;
    Job->Data = Data;
#ifdef DEBUG
    Job->JobName = JobName;
#endif
    
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
    s32 Handle = SubmitTexture(Memory, Width, Height);
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

PLATFORM_SET_SOCKET_OPTION(Win32SetSocketOption)
{
    s32 SocketIndex = (s32)SocketHandle;
    Assert(SocketIndex < Win32SocketHandleCount);
    int Error = setsockopt(SocketValues[SocketIndex].Socket, Level, Option, OptionValue, OptionLength);
    if (Error == SOCKET_ERROR)
    {
        Error = WSAGetLastError();
        // TODO(Peter): :ErrorLogging
    }
    
    return Error;
}

PLATFORM_GET_SOCKET_HANDLE(Win32GetSocketHandle)
{
    // NOTE(Peter): These used to be passed in as paramters, but we only use this function
    // with AF_INET, SOCK_DGRAM, and Protocol = 0. These are also platform specific values
    // so I was having to include windows.h in the platform agnostic code to accomodate that
    // function signature.
    s32 AddressFamily = AF_INET;
    s32 Type = SOCK_DGRAM;
    s32 Protocol = 0;
    
    if (Win32SocketHandleCount >= Win32SocketHandleMax)
    {
        s32 NewDictionaryMax = Win32SocketHandleMax + SOCKET_DICTIONARY_GROW_SIZE;
        s32 NewDictionaryDataSize = NewDictionaryMax *  sizeof(win32_socket);
        u8* DictionaryMemory = Win32Alloc(NewDictionaryDataSize);
        Assert(DictionaryMemory);
        
        win32_socket* NewValues = (win32_socket*)(DictionaryMemory);
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
    
    int Error = Win32SetSocketOption(NewSocketIndex, IPPROTO_IP, IP_MULTICAST_TTL,
                                     (const char*)(&Multicast_TimeToLive), sizeof(Multicast_TimeToLive));
    
    return (platform_socket_handle)NewSocketIndex;
}

PLATFORM_SEND_TO(Win32SendTo)
{
    s32 SocketIndex = (s32)SocketHandle;
    Assert(SocketIndex < Win32SocketHandleCount);
    
    sockaddr_in SockAddress = {};
    SockAddress.sin_family = AF_INET;
    SockAddress.sin_port = HostToNetU16(Port);
    SockAddress.sin_addr.s_addr = HostToNetU32(Address);
    
    s32 LengthSent = sendto(SocketValues[SocketIndex].Socket, Buffer, BufferLength, Flags, (sockaddr*)&SockAddress, sizeof(sockaddr_in));
    
    if (LengthSent == SOCKET_ERROR)
    {
        s32 Error = WSAGetLastError();
        if (Error == 10051)
        {
        }
        else
        {
            // TODO(Peter): :ErrorLogging
            InvalidCodePath;
        }
    }
    
    return LengthSent;
}

PLATFORM_CLOSE_SOCKET(Win32CloseSocket)
{
    s32 SocketIndex = (s32)SocketHandle;
    Assert(SocketIndex < Win32SocketHandleCount);
    
    closesocket(SocketValues[SocketIndex].Socket);
}

HDC FontDrawingDC;
HBITMAP FontBitmap;
HFONT CurrentFont;

GET_FONT_INFO(Win32GetFontInfo)
{
    platform_font_info Result = {};
    
    FontDrawingDC = CreateCompatibleDC(NULL);
    SetBkColor(FontDrawingDC, RGB(0, 0, 0));
    SetTextColor(FontDrawingDC, RGB(255, 255, 255));
    FontBitmap = CreateCompatibleBitmap(FontDrawingDC, PixelHeight * 2, PixelHeight * 2);
    HGDIOBJ SelectObjectResult = SelectObject(FontDrawingDC, FontBitmap);
    
    CurrentFont = CreateFont(PixelHeight, 0, 0, 0,
                             FontWeight,
                             Italic,
                             Underline,
                             Strikeout,
                             ANSI_CHARSET,
                             OUT_OUTLINE_PRECIS,
                             CLIP_DEFAULT_PRECIS,
                             PROOF_QUALITY,
                             FIXED_PITCH,
                             FontName);
    SelectFont(FontDrawingDC, CurrentFont);
    
    TEXTMETRIC WindowsFontMetrics = {};
    if (GetTextMetrics(FontDrawingDC, &WindowsFontMetrics))
    {
        Result.PixelHeight = WindowsFontMetrics.tmHeight;
        Result.Ascent = WindowsFontMetrics.tmAscent;
        Result.Descent      = WindowsFontMetrics.tmDescent;
        Result.Leading      = WindowsFontMetrics.tmExternalLeading;
        Result.MaxCharWidth = WindowsFontMetrics.tmMaxCharWidth;
        Result.CodepointStart = WindowsFontMetrics.tmFirstChar;
        Result.CodepointOnePastLast = WindowsFontMetrics.tmLastChar + 1;
    }
    
    return Result;
}

DRAW_FONT_CODEPOINT(Win32DrawFontCodepoint)
{
    SIZE CodepointSize = {};
    if (GetTextExtentPoint32(FontDrawingDC, &Codepoint, 1, &CodepointSize))
    {
        *OutWidth = CodepointSize.cx;
        *OutHeight = CodepointSize.cy;
        
        RECT TextRect = {};
        TextRect.left = 0;
        TextRect.right = *OutWidth;
        TextRect.top = 0;
        TextRect.bottom = *OutHeight;
        
        int Error = DrawText(FontDrawingDC, &Codepoint, 1, &TextRect, DT_LEFT | DT_NOCLIP | DT_TOP);
        
        u8* Row = DestBuffer + (YOffset * (DestBufferWidth * 4));
        COLORREF PixelColor;
        for (u32 Y = 0; Y < *OutHeight; Y++)
        {
            // NOTE(Peter): XOffset * 4 b/c its 4 bytes per pixel.
            u8* Channel = (u8*)Row + (XOffset * 4);
            for (u32 X = 0; X < *OutWidth; X++)
            {
                PixelColor = GetPixel(FontDrawingDC, X + TextRect.left, TextRect.bottom - Y);
                Assert(PixelColor != CLR_INVALID);
                u8 RValue = GetRValue(PixelColor);
                *Channel++ = RValue;
                *Channel++ = RValue;
                *Channel++ = RValue;
                *Channel++ = RValue;
            }
            Row += DestBufferWidth * 4;
        }
        
    }
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
HandleWindowMessage (MSG Message, window* Window, input_queue* InputQueue, mouse_state* Mouse)
{
    switch (Message.message)
    {
        case WM_MOUSEWHEEL:
        {
            Mouse->Scroll = GET_WHEEL_DELTA_WPARAM(Message.wParam);
        }break;
        
        case WM_LBUTTONDOWN:
        {
            b32 ShiftDown = GetKeyState(VK_SHIFT) & 0x8000;
            b32 AltDown = GetKeyState(VK_MENU) & 0x8000;
            b32 CtrlDown = GetKeyState(VK_CONTROL) & 0x8000;
            
            AddInputEventEntry(InputQueue, KeyCode_MouseLeftButton, false, true,
                               ShiftDown, AltDown, CtrlDown, false);
            
            Mouse->LeftButtonState = KeyState_IsDown & ~KeyState_WasDown;
            Mouse->DownPos = Mouse->Pos;
            
            // :Win32MouseEventCapture
            // NOTE(Peter): We capture events when the mouse goes down so that
            // if the user drags outside the window, we still get the mouse up
            // event and can process it. Otherwise, we can get into cases where
            // an event was started, didn't end, but the user can click again and
            // try to start the event again.
            // We relase event capture on mouse up.
            SetCapture(Window->Handle);
        }break;
        
        case WM_MBUTTONDOWN:
        {
            b32 ShiftDown = GetKeyState(VK_SHIFT) & 0x8000;
            b32 AltDown = GetKeyState(VK_MENU) & 0x8000;
            b32 CtrlDown = GetKeyState(VK_CONTROL) & 0x8000;
            
            AddInputEventEntry(InputQueue, KeyCode_MouseMiddleButton, false, true,
                               ShiftDown, AltDown, CtrlDown, false);
            Mouse->MiddleButtonState = KeyState_IsDown & ~KeyState_WasDown;
            
            // :Win32MouseEventCapture
            SetCapture(Window->Handle);
        }break;
        
        case WM_RBUTTONDOWN:
        {
            b32 ShiftDown = GetKeyState(VK_SHIFT) & 0x8000;
            b32 AltDown = GetKeyState(VK_MENU) & 0x8000;
            b32 CtrlDown = GetKeyState(VK_CONTROL) & 0x8000;
            
            AddInputEventEntry(InputQueue, KeyCode_MouseRightButton, false, true,
                               ShiftDown, AltDown, CtrlDown, false);
            Mouse->RightButtonState = KeyState_IsDown & ~KeyState_WasDown;
            Mouse->DownPos = Mouse->Pos;
            
            // :Win32MouseEventCapture
            SetCapture(Window->Handle);
        }break;
        
        case WM_LBUTTONUP:
        {
            b32 ShiftDown = GetKeyState(VK_SHIFT) & 0x8000;
            b32 AltDown = GetKeyState(VK_MENU) & 0x8000;
            b32 CtrlDown = GetKeyState(VK_CONTROL) & 0x8000;
            
            AddInputEventEntry(InputQueue, KeyCode_MouseLeftButton, true, false,
                               ShiftDown, AltDown, CtrlDown, false);
            Mouse->LeftButtonState = ~KeyState_IsDown & KeyState_WasDown;
            
            // :Win32MouseEventCapture
            ReleaseCapture();
        }break;
        
        case WM_MBUTTONUP:
        {
            b32 ShiftDown = GetKeyState(VK_SHIFT) & 0x8000;
            b32 AltDown = GetKeyState(VK_MENU) & 0x8000;
            b32 CtrlDown = GetKeyState(VK_CONTROL) & 0x8000;
            
            AddInputEventEntry(InputQueue, KeyCode_MouseMiddleButton, true, false,
                               ShiftDown, AltDown, CtrlDown, false);
            Mouse->MiddleButtonState = ~KeyState_IsDown & KeyState_WasDown;
            
            // :Win32MouseEventCapture
            ReleaseCapture();
        }break;
        
        case WM_RBUTTONUP:
        {
            b32 ShiftDown = GetKeyState(VK_SHIFT) & 0x8000;
            b32 AltDown = GetKeyState(VK_MENU) & 0x8000;
            b32 CtrlDown = GetKeyState(VK_CONTROL) & 0x8000;
            
            AddInputEventEntry(InputQueue, KeyCode_MouseRightButton, true, false,
                               ShiftDown, AltDown, CtrlDown, false);
            Mouse->RightButtonState = ~KeyState_IsDown & KeyState_WasDown;
            
            // :Win32MouseEventCapture
            ReleaseCapture();
        }break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            int VirtualKey = (int)Message.wParam;
            key_code Key = Win32GetKeyCode(VirtualKey, true, false);
            s32 KeyIndex = (int)Key;
            
            b32 KeyWasDown = (Message.lParam & (1 << 30)) != 0;
            b32 KeyIsDown = (Message.lParam & (1 << 31)) == 0;
            
            b32 ShiftDown = GetKeyState(VK_SHIFT) & 0x8000;
            b32 AltDown = GetKeyState(VK_MENU) & 0x8000;
            b32 CtrlDown = GetKeyState(VK_CONTROL) & 0x8000;
            
            // New Input Queue
            AddInputEventEntry(InputQueue, Key, KeyWasDown, KeyIsDown,
                               ShiftDown, AltDown, CtrlDown, false);
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
    string StringBuffer = MakeString(Buffer, 256);
    va_list Args;
    va_start(Args, Format);
    PrintF(&StringBuffer, Format, Args);
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

internal u8*
DEBUGAlloc(s32 ElementSize, s32 ElementCount)
{
    return Win32Alloc(ElementSize * ElementCount);
}

internal u8*
Win32Realloc(u8* Buf, s32 OldSize, s32 NewSize)
{
    u8* NewMemory = Win32Alloc(NewSize);
    GSMemCopy(Buf, NewMemory, OldSize);
    return NewMemory;
}

internal s32
Win32GetThreadId()
{
    s32 Result = GetCurrentThreadId();
    return Result;
}

// NOTE(Peter): Only meant to take one of the values specified below:
// IDC_APPSTARTING, IDC_ARROW, IDC_CROSS, IDC_HAND, IDC_HELP, IDC_IBEAM,
// IDC_ICON, IDC_NO, IDC_SIZE, IDC_SIZEALL, IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE,
// IDC_SIZEWE, IDC_UPARROW, IDC_WAIT
internal HCURSOR
Win32LoadSystemCursor(char* CursorIdentifier)
{
    u32 Error = 0;
    HCURSOR Result = LoadCursorA(NULL, CursorIdentifier);
    if (Result == NULL)
    {
        Error = GetLastError();
        InvalidCodePath;
    }
    return Result;
}

int WINAPI
WinMain (
         HINSTANCE HInstance,
         HINSTANCE HPrevInstance,
         PSTR CmdLineArgs,
         INT NCmdShow
         )
{
    MainWindow = Win32CreateWindow (HInstance, "Foldhaus", 1440, 768, HandleWindowEvents);
    Win32UpdateWindowDimension(&MainWindow);
    
    win32_opengl_window_info OpenGLWindowInfo = {};
    OpenGLWindowInfo.ColorBits = 32;
    OpenGLWindowInfo.AlphaBits = 8;
    OpenGLWindowInfo.DepthBits = 0;
    CreateOpenGLWindowContext(OpenGLWindowInfo, &MainWindow);
    
    s64 PerformanceCountFrequency = GetPerformanceFrequency();
    s64 LastFrameEnd = GetWallClock();
    r32 TargetSecondsPerFrame = 1 / 60.0f;
    r32 LastFrameSecondsElapsed = 0.0f;
    
    GlobalDebugServices = (debug_services*)malloc(sizeof(debug_services));
    s32 DebugThreadCount = PLATFORM_THREAD_COUNT + 1;
    InitDebugServices(GlobalDebugServices,
                      PerformanceCountFrequency,
                      DEBUGAlloc,
                      Win32Realloc,
                      GetWallClock,
                      Win32GetThreadId,
                      DebugThreadCount);
    
    input_queue InputQueue;
    {
        s32 InputQueueMemorySize = sizeof(input_entry) * 32;
        u8* InputQueueMemory = Win32Alloc(InputQueueMemorySize);
        InputQueue = InitializeInputQueue(InputQueueMemory, InputQueueMemorySize);
    }
    
    //
    // Set up worker threads
    const s32 WorkerThreadCount = PLATFORM_THREAD_COUNT;
    worker_thread_info* WorkerThreads = 0;
    if (PLATFORM_THREAD_COUNT > 0)
    {
        WorkerThreads = (worker_thread_info*)malloc(sizeof(worker_thread_info) * PLATFORM_THREAD_COUNT);
    }
    
    work_queue WorkQueue = {};
    WorkQueue.SemaphoreHandle = CreateSemaphoreEx(0, 0, PLATFORM_THREAD_COUNT, 0, 0, SEMAPHORE_ALL_ACCESS);
    WorkQueue.JobsMax = 512;
    WorkQueue.Jobs = (worker_thread_job*)Win32Alloc(sizeof(worker_thread_job) * WorkQueue.JobsMax);
    WorkQueue.NextJobIndex = 0;
    WorkQueue.PushWorkOnQueue = Win32PushWorkOnQueue;
    WorkQueue.DoQueueWorkUntilDone = Win32DoQueueWorkUntilDone;
    WorkQueue.ResetWorkQueue = ResetWorkQueue;
    
    OutputDebugStringA("Hellooooo\n");
    
    for (s32 i = 0; i < PLATFORM_THREAD_COUNT; i++)
    {
        // ID = 0 is reserved for this thread
        WorkerThreads[i].ID = i + 1;
        WorkerThreads[i].Queue = &WorkQueue;
        WorkerThreads[i].Handle = CreateThread(0, 0, &WorkerThreadProc, (void*)&WorkerThreads[i], 0, 0);
    }
    
    s32 InitialMemorySize = Megabytes(64);
    u8* InitialMemory = Win32Alloc(InitialMemorySize);
    context Context = {};
    Context.MemorySize = InitialMemorySize;
    Context.MemoryBase = InitialMemory;
    Context.WindowBounds = rect{v2{0, 0}, v2{(r32)MainWindow.Width, (r32)MainWindow.Height}};
    Context.Mouse = {0};
    
    // Cursors
    HCURSOR CursorArrow = Win32LoadSystemCursor(IDC_ARROW);
    HCURSOR CursorPointer = Win32LoadSystemCursor(IDC_HAND);
    HCURSOR CursorLoading = Win32LoadSystemCursor(IDC_WAIT);
    HCURSOR CursorHorizontalArrows = Win32LoadSystemCursor(IDC_SIZEWE);
    HCURSOR CursorVerticalArrows = Win32LoadSystemCursor(IDC_SIZENS);
    HCURSOR CursorDiagonalTopLeftArrows = Win32LoadSystemCursor(IDC_SIZENWSE);
    HCURSOR CursorDiagonalTopRightArrows = Win32LoadSystemCursor(IDC_SIZENESW);
    
    // Platform functions
    Context.GeneralWorkQueue = &WorkQueue;
    Context.PlatformAlloc = Win32Alloc;
    Context.PlatformFree = Win32Free;
    Context.PlatformRealloc = Win32Realloc;
    Context.FileHandler.ReadEntireFile = Win32ReadEntireFile;
    Context.FileHandler.WriteEntireFile = Win32WriteEntireFile;
    Context.FileHandler.GetFilePath = Win32SystemDialogueOpenFile;
    Context.PlatformGetGPUTextureHandle = Win32GetGPUTextureHandle;
    Context.PlatformGetSocketHandle = Win32GetSocketHandle;
    Context.PlatformSetSocketOption = Win32SetSocketOption;
    Context.PlatformSendTo = Win32SendTo;
    Context.PlatformCloseSocket = Win32CloseSocket;
    Context.PlatformGetFontInfo = Win32GetFontInfo;
    Context.PlatformDrawFontCodepoint = Win32DrawFontCodepoint;
    
    win32_dll_refresh DLLRefresh = InitializeDLLHotReloading(DLLName, WorkingDLLName, DLLLockFileName);
    if (HotLoadDLL(&DLLRefresh))
    {
        SetApplicationLinks(&Context, DLLRefresh, &WorkQueue);
        Context.ReloadStaticData(Context, GlobalDebugServices);
    }
    else
    {
        MessageBox(MainWindow.Handle, "Unable to load application DLL at startup.\nAborting\n", "Set Up Error", MB_OK);
        return -1;
    }
    
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);
    
    s32 RenderMemorySize = Megabytes(12);
    u8* RenderMemory = Win32Alloc(RenderMemorySize);
    render_command_buffer RenderBuffer = AllocateRenderCommandBuffer(RenderMemory, RenderMemorySize, Win32Realloc);
    
    Context.InitializeApplication(Context);
    
    Running = true;
    Context.WindowIsVisible = true;
    while (Running)
    {
        if (GlobalDebugServices->RecordFrames)
        {
            EndDebugFrame(GlobalDebugServices);
        }
        DEBUG_TRACK_SCOPE(MainLoop);
        
        ResetInputQueue(&InputQueue);
        if (HotLoadDLL(&DLLRefresh))
        {
            SetApplicationLinks(&Context, DLLRefresh, &WorkQueue);
            Context.ReloadStaticData(Context, GlobalDebugServices);
        }
        
        { // Mouse Position
            POINT MousePos;
            GetCursorPos (&MousePos);
            ScreenToClient(MainWindow.Handle, &MousePos);
            
            Context.Mouse.Scroll = 0;
            Context.Mouse.OldPos = Context.Mouse.Pos;
            Context.Mouse.Pos = v2{(r32)MousePos.x, (r32)MainWindow.Height - MousePos.y};
            Context.Mouse.DeltaPos = Context.Mouse.Pos - Context.Mouse.OldPos;
        }
        
        MSG Message;
        while (PeekMessageA(&Message, MainWindow.Handle, 0, 0, PM_REMOVE))
        {
            DEBUG_TRACK_SCOPE(PeekWindowsMessages);
            HandleWindowMessage(Message, &MainWindow, &InputQueue, &Context.Mouse);
        }
        
        Context.WindowBounds = rect{v2{0, 0}, v2{(r32)MainWindow.Width, (r32)MainWindow.Height}};
        RenderBuffer.ViewWidth = MainWindow.Width;
        RenderBuffer.ViewHeight = MainWindow.Height;
        Context.DeltaTime = LastFrameSecondsElapsed;
        
        Context.UpdateAndRender(&Context, InputQueue, &RenderBuffer);
        
        RenderCommandBuffer(RenderBuffer);
        ClearRenderBuffer(&RenderBuffer);
        
        Context.Mouse.LeftButtonState = GetMouseButtonStateAdvanced(Context.Mouse.LeftButtonState);
        Context.Mouse.MiddleButtonState = GetMouseButtonStateAdvanced(Context.Mouse.MiddleButtonState);
        Context.Mouse.RightButtonState = GetMouseButtonStateAdvanced(Context.Mouse.RightButtonState);
        
        switch (Context.Mouse.CursorType)
        {
            case CursorType_Arrow:
            {
                SetCursor(CursorArrow);
            }break;
            case CursorType_Pointer:
            {
                SetCursor(CursorPointer);
            }break;
            case CursorType_Loading:
            {
                SetCursor(CursorLoading);
            }break;
            case CursorType_HorizontalArrows:
            {
                SetCursor(CursorHorizontalArrows);
            }break;
            case CursorType_VerticalArrows:
            {
                SetCursor(CursorVerticalArrows);
            }break;
            case CursorType_DiagonalTopLeftArrows:
            {
                SetCursor(CursorDiagonalTopLeftArrows);
            }break;
            case CursorType_DiagonalTopRightArrows:
            {
                SetCursor(CursorDiagonalTopRightArrows);
            }break;
            InvalidDefaultCase;
        }
        
        HDC DeviceContext = GetDC(MainWindow.Handle);
        SwapBuffers(DeviceContext);
        ReleaseDC(MainWindow.Handle, DeviceContext);
        
        s64 FinishedWorkTime = GetWallClock();
        r32 SecondsElapsed = GetSecondsElapsed(LastFrameEnd, FinishedWorkTime, PerformanceCountFrequency);
        
        while (SecondsElapsed < TargetSecondsPerFrame)
        {
            DEBUG_TRACK_SCOPE(UnusedTime);
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
    
    for (s32 Thread = 0; Thread < PLATFORM_THREAD_COUNT; Thread++)
    {
        TerminateThread(WorkerThreads[Thread].Handle, 0);
    }
    
    return 0;
}

#define WIN32_FOLDHAUS_CPP
#endif // WIN32_FOLDHAUS_CPP