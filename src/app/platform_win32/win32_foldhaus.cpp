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

#include "../../meta/gs_meta_include.h"
#include "../foldhaus_platform.h"

#include "../../gs_libs/gs_win32.cpp"
#include "win32_foldhaus_utils.h"
#include "win32_foldhaus_memory.h"
#include "win32_foldhaus_fileio.h"
#include "win32_foldhaus_dll.h"
#include "win32_foldhaus_timing.h"
#include "win32_foldhaus_work_queue.h"
#include "win32_foldhaus_serial.h"
#include "win32_foldhaus_socket.h"
#include "win32_foldhaus_mouse.h"

#include "../foldhaus_renderer.cpp"

#include "win32_test_code.cpp"

global b32 Running = false;
global b32 WindowIsActive = false;

char DLLName[] = "foldhaus.dll";
char WorkingDLLName[] = "foldhaus_temp.dll";
char DLLLockFileName[] = "lock.tmp";

window MainWindow;

PLATFORM_GET_GPU_TEXTURE_HANDLE(Win32GetGPUTextureHandle)
{
    s32 Handle = SubmitTexture(Memory, Width, Height);
    return Handle;
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
            
            Mouse->LeftButtonState |= KeyState_IsDown;
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
            Mouse->LeftButtonState &= ~KeyState_IsDown;
            
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
#if 0
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
#endif
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }break;
        
        case WM_CHAR:
        {
            char VirtualKey = (char)Message.wParam;
            key_code Key = CharToKeyCode(VirtualKey);
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
    gs_string StringBuffer = MakeString(Buffer, 256);
    va_list Args;
    va_start(Args, Format);
    PrintF(&StringBuffer, Format, Args);
    OutputDebugStringA(Buffer);
    va_end(Args);
}

internal void
SetApplicationLinks (context* Context, win32_dll_refresh DLL, gs_work_queue* WorkQueue)
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

// TODO(Peter): :Redundant remove
internal u8*
DEBUGAlloc(s32 ElementSize, s32 ElementCount)
{
    return (u8*)Win32Alloc(ElementSize * ElementCount, 0);
}

// TODO(Peter): :Redundant remove
internal u8*
Win32Realloc(u8* Buf, s32 OldSize, s32 NewSize)
{
    u8* NewMemory = (u8*)Win32Alloc(NewSize, 0);
    CopyMemoryTo(Buf, NewMemory, OldSize);
    return NewMemory;
}

internal void
Win32_SendAddressedDataBuffer(gs_thread_context Context, addressed_data_buffer* BufferAt)
{
    DEBUG_TRACK_FUNCTION;
    
    u32 BuffersSent = 0;
    u32 DataSizeSent = 0;
    
    switch(BufferAt->AddressType)
    {
        case AddressType_NetworkIP:
        {
            Win32Socket_SendTo(BufferAt->SendSocket,
                               BufferAt->V4SendAddress,
                               BufferAt->SendPort,
                               (const char*)BufferAt->Memory,
                               BufferAt->MemorySize,
                               0);
        }break;
        
        case AddressType_ComPort:
        {
            if (BufferAt->ComPort.Length > 0)
            {
                HANDLE SerialPort = Win32SerialArray_GetOrOpen(BufferAt->ComPort, 2000000, 8, NOPARITY, 1);
                if (SerialPort != INVALID_HANDLE_VALUE)
                {
                    if (Win32SerialPort_Write(SerialPort, BufferAt->Data))
                    {
                        BuffersSent += 1;
                        DataSizeSent += BufferAt->Data.Size;
                    }
                    else
                    {
                        Win32SerialArray_Close(BufferAt->ComPort);
                    }
                }
            }
            else
            {
#if 0
                OutputDebugStringA("Skipping data buffer because its COM Port isn't set");
#endif
            }
        }break;
        
        InvalidDefaultCase;
    }
}

internal void
Win32_SendAddressedDataBuffer_Job(gs_thread_context Context, gs_data Arg)
{
    addressed_data_buffer* OutputData = (addressed_data_buffer*)Arg.Memory;
    Win32_SendAddressedDataBuffer(Context, OutputData);
}

internal bool
ReloadAndLinkDLL(win32_dll_refresh* DLL, context* Context, gs_work_queue* WorkQueue, bool ShouldError)
{
    bool Success = false;
    if (HotLoadDLL(DLL))
    {
        SetApplicationLinks(Context, *DLL, WorkQueue);
        Context->ReloadStaticData(*Context, GlobalDebugServices);
        Success = true;
    }
    else if(ShouldError)
    {
        OutputDebugStringA("Unable to load application DLL at startup.\nAborting\n");
    }
    return Success;
}

internal gs_const_string
GetExePath(HINSTANCE HInstance, gs_thread_context ThreadContext)
{
    gs_const_string Result = {};
    
    u32 Error = 0;
    u32 PathSize = MAX_PATH;
    char* Path = PushArray(ThreadContext.Transient, char, PathSize);
    DWORD Length = GetModuleFileNameA(HInstance, Path, PathSize);
    
    if (Length)
    {
        Error = GetLastError();
        if (Error == ERROR_INSUFFICIENT_BUFFER) {
            // PathSize wasn't long enough
            // TODO(pjs): handle this case
            InvalidCodePath;
        }
        
        Result.Str = Path;
        Result.Length = (u64)Length;
    }
    else
    {
        Error = GetLastError();
        InvalidCodePath;
    }
    
    return Result;
}

internal bool
SetWorkingDirectory(HINSTANCE HInstance, gs_thread_context ThreadContext)
{
    bool Result = false;
    
    gs_const_string ExePath = GetExePath(HInstance, ThreadContext);
    gs_string ScratchPath = PushString(ThreadContext.Transient, ExePath.Length + 128);
    gs_string WorkingDirectory = PushString(ThreadContext.Transient, ExePath.Length + 128);
    
    while (WorkingDirectory.Length == 0)
    {
        s64 LastSlash = FindLastFromSet(ExePath, "\\/");
        if (LastSlash < 0) break;
        
        ExePath = Substring(ExePath, 0, LastSlash);
        PrintF(&ScratchPath, "%S\\data", ExePath);
        NullTerminate(&ScratchPath);
        
        gs_file_info PathInfo = GetFileInfo(ThreadContext.FileHandler, ScratchPath.ConstString);
        if (PathInfo.Path.Length > 0 &&
            PathInfo.IsDirectory) {
            PrintF(&WorkingDirectory, "%S", ExePath);
            NullTerminate(&WorkingDirectory);
        }
    }
    
    if (WorkingDirectory.Length > 0)
    {
        OutputDebugStringA("Setting Working Directory\n");
        OutputDebugStringA(WorkingDirectory.Str);
        OutputDebugStringA("\n");
        Result = SetCurrentDirectory(WorkingDirectory.Str);
        if (!Result)
        {
            u32 Error = GetLastError();
            InvalidCodePath;
        }
    }
    else
    {
        OutputDebugStringA("Error, no data folder found\n");
    }
    
    return Result;
}

#include "../../gs_libs/gs_path.h"

#include "../../gs_libs/gs_csv.h"

int WINAPI
WinMain (
         HINSTANCE HInstance,
         HINSTANCE HPrevInstance,
         PSTR CmdLineArgs,
         INT NCmdShow
         )
{
    gs_thread_context ThreadContext = Win32CreateThreadContext();
    
    gs_allocator_debug AllocDebug = {};
    AllocDebug.AllocationsCountMax = 4096;
    AllocDebug.Allocations = (gs_debug_allocation*)Win32Alloc(sizeof(gs_debug_allocation) * AllocDebug.AllocationsCountMax, 0);
    
    ThreadContext.Allocator.Debug = &AllocDebug;
    
    if (!SetWorkingDirectory(HInstance, ThreadContext)) return 1;
    
    
    gs_file TestFile = ReadEntireFile(ThreadContext.FileHandler, ConstString("data/flower_codes.tsv"));
    gs_const_string TestFileStr = {};
    TestFileStr.Str = (char*)TestFile.Memory;
    TestFileStr.Length = TestFile.Size;
    gscsv_sheet Sheet = CSV_Parse(TestFileStr, { '\t' }, ThreadContext.Transient);
    
    gs_string Out = PushString(ThreadContext.Transient, TestFile.Size * 2);
    
    for (u64 y = 0; y < Sheet.RowCount; y++)
    {
        for (u64 x = 0; x < Sheet.ColumnCount; x++)
        {
            gs_const_string Cell = CSVSheet_GetCell(Sheet, x, y);
            AppendPrintF(&Out, "%S\t", Cell);
        }
        AppendPrintF(&Out, "\n");
    }
    NullTerminate(&Out);
    OutputDebugStringA(Out.Str);
    
    MainWindow = Win32CreateWindow (HInstance, "Foldhaus", 1440, 768, HandleWindowEvents);
    Win32UpdateWindowDimension(&MainWindow);
    
    win32_opengl_window_info OpenGLWindowInfo = {};
    OpenGLWindowInfo.ColorBits = 32;
    OpenGLWindowInfo.AlphaBits = 8;
    OpenGLWindowInfo.DepthBits = 0;
    CreateOpenGLWindowContext(OpenGLWindowInfo, &MainWindow);
    
    context Context = {};
    Context.ThreadContext = ThreadContext;
    Context.MemorySize = MB(64);
    Context.MemoryBase = (u8*)Win32Alloc(Context.MemorySize, 0);
    
    gs_memory_arena PlatformPermanent = CreateMemoryArena(Context.ThreadContext.Allocator, "Platform Memory");
    
    s64 PerformanceCountFrequency = GetPerformanceFrequency();
    s64 LastFrameEnd = GetWallClock();
    r32 TargetSecondsPerFrame = 1 / 60.0f;
    r32 LastFrameSecondsElapsed = 0.0f;
    
    GlobalDebugServices = PushStruct(&PlatformPermanent, debug_services);
    InitDebugServices(GlobalDebugServices,
                      PerformanceCountFrequency,
                      DEBUGAlloc,
                      Win32Realloc,
                      GetWallClock,
                      Win32GetThreadId,
                      PLATFORM_THREAD_COUNT + 1);
    
    input_queue InputQueue = InputQueue_Create(&PlatformPermanent, 32);
    
    Win32WorkQueue_Init(&PlatformPermanent, PLATFORM_THREAD_COUNT);
    
    // Platform functions
    Context.GeneralWorkQueue = &Win32WorkQueue.WorkQueue;
    Context.PlatformGetGPUTextureHandle = Win32GetGPUTextureHandle;
    Context.PlatformGetSocketHandle = Win32GetSocketHandle;
    Context.PlatformGetFontInfo = Win32GetFontInfo;
    Context.PlatformDrawFontCodepoint = Win32DrawFontCodepoint;
    
    Context.ThreadManager = PushStruct(&PlatformPermanent, platform_thread_manager);
    *Context.ThreadManager = CreatePlatformThreadManager(Win32CreateThread, Win32KillThread);
    
    Context.SocketManager = PushStruct(&PlatformPermanent, platform_socket_manager);
    *Context.SocketManager = CreatePlatformSocketManager(Win32ConnectSocket, Win32CloseSocket, Win32SocketQueryStatus, Win32SocketPeek, Win32SocketReceive, Win32SocketSend);
    
    win32_dll_refresh DLLRefresh = InitializeDLLHotReloading(DLLName, WorkingDLLName, DLLLockFileName);
    if (!ReloadAndLinkDLL(&DLLRefresh, &Context, &Win32WorkQueue.WorkQueue, true)) { return -1; }
    
    Mouse_Init();
    
    Win32SocketSystem_Init(&PlatformPermanent);
    
    Win32SerialArray_Create(ThreadContext);
    
    render_command_buffer RenderBuffer = AllocateRenderCommandBuffer(MB(12), &PlatformPermanent, Win32Realloc);
    
    addressed_data_buffer_list OutputData = AddressedDataBufferList_Create(ThreadContext);
    
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
        
        {
            // update system time
            SYSTEMTIME WinLocalTime;
            GetLocalTime(&WinLocalTime);
            
            SYSTEMTIME WinSysTime;
            FILETIME WinSysFileTime;
            GetSystemTime(&WinSysTime);
            if (!SystemTimeToFileTime((const SYSTEMTIME*)&WinSysTime, &WinSysFileTime))
            {
                u32 Error = GetLastError();
                InvalidCodePath;
            }
            ULARGE_INTEGER SysTime = {};
            SysTime.LowPart = WinSysFileTime.dwLowDateTime;
            SysTime.HighPart = WinSysFileTime.dwHighDateTime;
            
            Context.SystemTime_Last = Context.SystemTime_Current;
            
            Context.SystemTime_Current.NanosSinceEpoch = SysTime.QuadPart;
            Context.SystemTime_Current.Year = WinLocalTime.wYear;
            Context.SystemTime_Current.Month = WinLocalTime.wMonth;
            Context.SystemTime_Current.Day = WinLocalTime.wDay;
            Context.SystemTime_Current.Hour = WinLocalTime.wHour;
            Context.SystemTime_Current.Minute = WinLocalTime.wMinute;
            Context.SystemTime_Current.Second = WinLocalTime.wSecond;
            
#define PRINT_SYSTEM_TIME 0
#if PRINT_SYSTEM_TIME
            gs_string T = PushStringF(Context.ThreadContext.Transient,
                                      256,
                                      "%d %d %d - %lld\n",
                                      Context.SystemTime_Current.Hour,
                                      Context.SystemTime_Current.Minute,
                                      Context.SystemTime_Current.Second,
                                      Context.SystemTime_Current.NanosSinceEpoch);
            NullTerminate(&T);
            OutputDebugStringA(T.Str);
#endif
        }
        
        ResetInputQueue(&InputQueue);
        
        ReloadAndLinkDLL(&DLLRefresh, &Context, &Win32WorkQueue.WorkQueue, false);
        
        AddressedDataBufferList_Clear(&OutputData);
        
        Mouse_Update(MainWindow, &Context);
        
        MSG Message;
        while (PeekMessageA(&Message, MainWindow.Handle, 0, 0, PM_REMOVE))
        {
            DEBUG_TRACK_SCOPE(PeekWindowsMessages);
            HandleWindowMessage(Message, &MainWindow, &InputQueue, &Context.Mouse);
        }
        
        Context.WindowBounds = rect2{v2{0, 0}, v2{(r32)MainWindow.Width, (r32)MainWindow.Height}};
        RenderBuffer.ViewWidth = MainWindow.Width;
        RenderBuffer.ViewHeight = MainWindow.Height;
        Context.DeltaTime = LastFrameSecondsElapsed;
        
#if 0
        gs_string T = PushStringF(Context.ThreadContext.Transient, 256, "%f\n", Context.DeltaTime);
        NullTerminate(&T);
        OutputDebugStringA(T.Str);
#endif
        
        Context.UpdateAndRender(&Context, InputQueue, &RenderBuffer, &OutputData);
        
        bool Multithread = false;
        if (Multithread)
        {
            for (addressed_data_buffer* At = OutputData.Root;
                 At != 0;
                 At = At->Next)
            {
                gs_data ProcArg = {};
                ProcArg.Memory = (u8*)At;
                ProcArg.Size = sizeof(addressed_data_buffer);
                Win32PushWorkOnQueue(&Win32WorkQueue.WorkQueue, Win32_SendAddressedDataBuffer_Job, ProcArg, ConstString("Send UART Data"));
            }
        }
        else
        {
            for (addressed_data_buffer* At = OutputData.Root;
                 At != 0;
                 At = At->Next)
            {
                gs_data ProcArg = {};
                ProcArg.Memory = (u8*)At;
                ProcArg.Size = sizeof(addressed_data_buffer);
                Win32_SendAddressedDataBuffer_Job(ThreadContext, ProcArg);
            }
        }
        
        RenderCommandBuffer(RenderBuffer);
        ClearRenderBuffer(&RenderBuffer);
        
        Mouse_Advance(&Context);
        
        HDC DeviceContext = GetDC(MainWindow.Handle);
        SwapBuffers(DeviceContext);
        ReleaseDC(MainWindow.Handle, DeviceContext);
        
        Win32DoQueueWorkUntilDone(&Win32WorkQueue.WorkQueue, Context.ThreadContext);
        
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
    
    Win32WorkQueue_Cleanup();
    //Win32_TestCode_SocketReading_Cleanup();
    
    Win32SocketSystem_Cleanup();
    
    return 0;
}

#define WIN32_FOLDHAUS_CPP
#endif // WIN32_FOLDHAUS_CPP