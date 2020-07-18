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
#include "win32_foldhaus_dll.h"
#include "win32_foldhaus_timing.h"

#include "foldhaus_renderer.cpp"

global b32 Running = false;
global b32 WindowIsActive = false;

char DLLName[] = "foldhaus.dll";
char WorkingDLLName[] = "foldhaus_temp.dll";
char DLLLockFileName[] = "lock.tmp";

window MainWindow;

// Utils

internal gs_string
Win32DumpErrorAndPrepareMessageBoxString(gs_memory_arena* Arena, char* Format, ...)
{
    s32 Error = GetLastError();
    gs_string ErrorDump = PushString(Arena, 4096);
    PrintF(&ErrorDump,
           R"FOO(Win32 Error: %s\n
Error Code: %d\n
           )FOO",
           __FUNCTION__,
           Error);
    
    va_list Args;
    va_start(Args, Format);
    PrintFArgsList(&ErrorDump, Format, Args);
    va_end(Args);
    
    gs_data ErrorDumpData = StringToData(ErrorDump);
    
    HANDLE FileHandle = CreateFileA("./crashdump.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten = 0;
        if (WriteFile(FileHandle, ErrorDumpData.Memory, ErrorDumpData.Size, &BytesWritten, NULL))
        {
            
        }
        CloseHandle(FileHandle);
    }
    
    AppendPrintF(&ErrorDump, "Program will attempt to continue. See crashdump.txt for info");
    NullTerminate(&ErrorDump);
    
    return ErrorDump;
}

DEBUG_PRINT(Win32DebugPrint)
{
    Assert(IsNullTerminated(Message));
    OutputDebugStringA(Message.Str);
}

#define PrintLastError() PrintLastError_(__FILE__, __LINE__)
internal void
PrintLastError_(char* File, u32 Line)
{
    char DebugStringData[256];
    gs_string DebugString = MakeString(DebugStringData, 0, 256);
    u32 Error = GetLastError();
    PrintF(&DebugString, "%s Line %d: Win32 Error %d\n\0", File, Line, Error);
    OutputDebugStringA(DebugString.Str);
}


internal HINSTANCE
GetHInstance()
{
    HINSTANCE Result = GetModuleHandle(NULL);
    if (Result == NULL)
    {
        PrintLastError();
    }
    return Result;
}

///////////////////////
//
//   Fie I/O

internal u64
Win32HighLowToU64(u32 LowPart, u32 HighPart)
{
    ULARGE_INTEGER Time = {};
    Time.LowPart = LowPart;
    Time.HighPart = HighPart;
    u64 Result = Time.QuadPart;
    return Result;
}

internal u64
Win32FileTimeToU64(FILETIME FileTime)
{
    u64 Result = Win32HighLowToU64(FileTime.dwLowDateTime, FileTime.dwHighDateTime);
    return Result;
}

GET_FILE_INFO(Win32GetFileInfo)
{
    Assert(IsNullTerminated(Path));
    gs_file_info Result = {};
    HANDLE FileHandle = CreateFileA(Path.Str, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        Result.Path = Path;
        Result.FileSize = (u64)GetFileSize(FileHandle, NULL) + 1;
        FILETIME CreationTime, LastWriteTime;
        if (GetFileTime(FileHandle, &CreationTime, (LPFILETIME)0, &LastWriteTime))
        {
            Result.CreationTime = Win32FileTimeToU64(CreationTime);
            Result.LastWriteTime = Win32FileTimeToU64(LastWriteTime);
        }
        else
        {
            PrintLastError();
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

READ_ENTIRE_FILE(Win32ReadEntireFile)
{
    Assert(DataIsNonEmpty(Memory));
    Assert(IsNullTerminated(Path));
    
    gs_file Result = {0};
    u32 Error = 0;
    Result.FileInfo.Path = Path;
    
    HANDLE FileHandle = CreateFileA(Path.Str, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesRead = 0;
        if (ReadFile(FileHandle, (LPVOID)Memory.Memory, Memory.Size - 1, (LPDWORD)(&BytesRead), NULL))
        {
            Memory.Memory[Memory.Size - 1] = 0;
            Result.Data = Memory;
            
            gs_string AbsolutePath = PushString(FileHandler.Transient, 512);
            AbsolutePath.Length = GetFullPathNameA(Path.Str, AbsolutePath.Size, AbsolutePath.Str, NULL);
            if (AbsolutePath.Length == 0)
            {
                Error = GetLastError();
                InvalidCodePath;
            }
            Result.FileInfo.AbsolutePath = AbsolutePath.ConstString;
        }
        else
        {
            // NOTE(Peter): If we get to this error case, it means that the file exists,
            // and was successfully opened, but we can't read from it. I'm choosing to
            // treat this as a legitimate error at this point.
            gs_string Message = Win32DumpErrorAndPrepareMessageBoxString(FileHandler.Transient, "Attempting to read file: %S", Path);
            if (MessageBox(NULL, Message.Str, "Error", MB_OK) == IDOK)
            {
                PostQuitMessage(-1);
            }
        }
        CloseHandle(FileHandle);
    }
    else
    {
        
    }
    
    return Result;
}

WRITE_ENTIRE_FILE(Win32WriteEntireFile)
{
    Assert(DataIsNonEmpty(Data));
    Assert(IsNullTerminated(Path));
    
    bool Success = false;
    HANDLE FileHandle = CreateFileA(Path.Str, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle  != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten = 0;
        if (WriteFile(FileHandle, Data.Memory, Data.Size, &BytesWritten, NULL))
        {
            Success = (BytesWritten == Data.Size);
        }
        else
        {
            gs_string Message = Win32DumpErrorAndPrepareMessageBoxString(FileHandler.Transient, "Attempting to write to file: %S", Path);
            MessageBox(NULL, Message.Str, "Error", MB_OK);
        }
        CloseHandle(FileHandle);
    }
    else
    {
        
    }
    
    return Success;
}

internal FILETIME
GetFileLastWriteTime(char* Path)
{
    FILETIME Result = {};
    
    WIN32_FIND_DATA FindData = {};
    HANDLE FileHandle = FindFirstFileA(Path, &FindData);
    
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        Result = FindData.ftLastWriteTime;
        FindClose(FileHandle);
    }
    else
    {
        // TODO(Peter): :ErrorLogging
    }
    
    return Result;
}

struct temp_file_list_entry
{
    gs_file_info Info;
    temp_file_list_entry* Next;
};

struct temp_file_list
{
    temp_file_list_entry* First;
    temp_file_list_entry* Last;
};

internal u32
Win32EnumerateDirectoryIntoTempList(gs_file_handler FileHandler, temp_file_list* TempList, gs_const_string Path, gs_memory_arena* Storage, b32 Flags)
{
    u32 FilesCount = 0;
    
    u32 IndexOfLastSlash = FindLastFromSet(Path, "\\/");
    gs_const_string SearchPath = Substring(Path, 0, IndexOfLastSlash + 1);
    
    WIN32_FIND_DATA FindFileData;
    HANDLE SearchHandle = FindFirstFile(Path.Str, &FindFileData);
    if (SearchHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (HasFlag(FindFileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                if (HasFlag(Flags, EnumerateDirectory_Recurse))
                {
                    gs_const_string SubDirName = ConstString(FindFileData.cFileName);
                    if (!StringsEqual(SubDirName, ConstString(".")) &&
                        !StringsEqual(SubDirName, ConstString("..")))
                    {
                        gs_string SubDirectoryPath = PushString(FileHandler.Transient, SearchPath.Length + SubDirName.Length + 3);
                        PrintF(&SubDirectoryPath, "%S%S/*\0", SearchPath, SubDirName);
                        FilesCount += Win32EnumerateDirectoryIntoTempList(FileHandler, TempList, SubDirectoryPath.ConstString, Storage, Flags);
                    }
                }
                
                if (HasFlag(Flags, EnumerateDirectory_IncludeDirectories))
                {
                    FilesCount += 1;
                }
            }
            else
            {
                temp_file_list_entry* File = PushStruct(FileHandler.Transient, temp_file_list_entry);
                File->Info.FileSize = Win32HighLowToU64(FindFileData.nFileSizeLow, FindFileData.nFileSizeHigh);
                File->Info.CreationTime = Win32FileTimeToU64(FindFileData.ftCreationTime);
                File->Info.LastWriteTime = Win32FileTimeToU64(FindFileData.ftLastWriteTime);
                File->Next = 0;
                
                u32 FileNameLength = CharArrayLength(FindFileData.cFileName);
                
                // NOTE(Peter): String Storage
                // Storing the string in the final storage means we don't have to copy the string later, and all
                // strings will be continguous in memory at the calling site, though they will be before the array
                gs_string FileName = PushString(Storage, SearchPath.Length + FileNameLength + 1);
                PrintF(&FileName, "%S%.*s", SearchPath, FileName.Size, FindFileData.cFileName);
                NullTerminate(&FileName);
                File->Info.Path = FileName.ConstString;
                
                SLLPushOrInit(TempList->First, TempList->Last, File);
                FilesCount += 1;
            }
        }while(FindNextFile(SearchHandle, &FindFileData));
    }
    else
    {
        PrintLastError();
    }
    
    return FilesCount;
}

ENUMERATE_DIRECTORY(Win32EnumerateDirectory)
{
    Assert(IsNullTerminated(Path));
    gs_file_info_array Result = {};
    
    temp_file_list TempList = {};
    Result.MaxCount = Win32EnumerateDirectoryIntoTempList(FileHandler, &TempList, Path, Storage, Flags);
    
    Result.Values = PushArray(Storage, gs_file_info, Result.MaxCount);
    for (temp_file_list_entry* FileAt = TempList.First;
         FileAt != 0;
         FileAt = FileAt->Next)
    {
        // NOTE(Peter): We don't copy the file name here because its already in Storage.
        // See String Storage note above ^^
        Result.Values[Result.Count++] = FileAt->Info;
    }
    
    return Result;
}

///////////////////////
//
//   Job System

struct worker_thread_entry
{
    b32 IsValid;
    u32 Index;
};

struct worker_thread_info
{
    gs_thread_context ThreadContext;
    HANDLE Handle;
    gs_work_queue* Queue;
};

internal s32
Win32GetThreadId()
{
    s32 Result = GetCurrentThreadId();
    return Result;
}

internal gs_thread_context
Win32CreateThreadContext(gs_memory_arena* Transient = 0)
{
    gs_thread_context Result = {0};
    Result.ThreadInfo.ThreadID = Win32GetThreadId();
    Result.Allocator = CreateAllocator(Win32Alloc, Win32Free);
    if (Transient != 0)
    {
        Result.Transient = Transient;
    }
    else
    {
        Result.Transient = (gs_memory_arena*)AllocatorAlloc(Result.Allocator, sizeof(gs_memory_arena)).Memory;
        *Result.Transient = CreateMemoryArena(Result.Allocator);
    }
    Result.FileHandler = CreateFileHandler(Win32GetFileInfo,
                                           Win32ReadEntireFile,
                                           Win32WriteEntireFile,
                                           Win32EnumerateDirectory,
                                           Result.Transient);
    
    return Result;
}

PUSH_WORK_ON_QUEUE(Win32PushWorkOnQueue)
{
#ifdef DEBUG
    // NOTE(Peter): Just prints out the names of all the pending jobs if we end up
    // overflowing the buffer
    if (Queue->JobsCount >= Queue->JobsMax)
    {
        gs_string DebugString = MakeString((char*)malloc(256), 256);
        for (u32 i = 0; i < Queue->JobsCount; i++)
        {
            PrintF(&DebugString, "%d %s\n", i, Queue->Jobs[i].JobName);
            NullTerminate(&DebugString);
            OutputDebugStringA(DebugString.Str);
        }
    }
#endif
    Assert(Queue->JobsCount < Queue->JobsMax);
    
    gs_threaded_job* Job = Queue->Jobs + Queue->JobsCount;
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
CompleteAndTakeNextJob(gs_work_queue* Queue, worker_thread_entry Completed, gs_thread_context Context)
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

COMPLETE_QUEUE_WORK(Win32DoQueueWorkUntilDone)
{
    worker_thread_entry Entry = {};
    Entry.IsValid = false;
    while (Queue->JobsCompleted < Queue->JobsCount)
    {
        Entry = CompleteAndTakeNextJob(Queue, Entry, Context);
        if (Entry.IsValid)
        {
            Queue->Jobs[Entry.Index].WorkProc(Context, Queue->Jobs[Entry.Index].Data);
        }
    }
}

DWORD WINAPI
WorkerThreadProc (LPVOID InputThreadInfo)
{
    worker_thread_info* ThreadInfo = (worker_thread_info*)InputThreadInfo;
    ThreadInfo->ThreadContext = Win32CreateThreadContext();
    
    worker_thread_entry Entry = {};
    Entry.IsValid = false;
    while (true)
    {
        ClearArena(ThreadInfo->ThreadContext.Transient);
        Entry = CompleteAndTakeNextJob(ThreadInfo->Queue, Entry, ThreadInfo->ThreadContext);
        if (Entry.IsValid)
        {
            ThreadInfo->Queue->Jobs[Entry.Index].WorkProc(ThreadInfo->ThreadContext,
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
        u8* DictionaryMemory = (u8*)Win32Alloc(NewDictionaryDataSize, 0);
        Assert(DictionaryMemory);
        
        win32_socket* NewValues = (win32_socket*)(DictionaryMemory);
        if (SocketValues)
        {
            CopyMemoryTo(SocketValues, NewValues, sizeof(win32_socket) * NewDictionaryMax);
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

internal void
PrintMatrix(m44 M, gs_thread_context Context)
{
    gs_string PrintString = AllocatorAllocString(Context.Allocator, 256);
    PrintF(&PrintString, "[\n    %f %f %f %f\n    %f %f %f %f\n    %f %f %f %f\n    %f %f %f %f\n]\n",
           M.Array[0], M.Array[1], M.Array[2], M.Array[3],
           M.Array[4], M.Array[5], M.Array[6], M.Array[7],
           M.Array[8], M.Array[9], M.Array[10], M.Array[11],
           M.Array[12], M.Array[13], M.Array[14], M.Array[15]);
    NullTerminate(&PrintString);
    OutputDebugStringA(PrintString.Str);
}

v4 PerspectiveDivide(v4 A)
{
    v4 Result = {0, 0, 0, 1};
    Result.x = A.x / A.w;
    Result.y = A.y / A.w;
    Result.z = A.z / A.w;
    Result.w = A.w;
    return Result;
}
v4 ToScreen(v4 P, rect2 WindowBounds)
{
    v4 Result = P;
    Result.x = RemapR32(P.x, -1, 1, WindowBounds.Min.x, WindowBounds.Max.x);
    Result.y = RemapR32(P.y, -1, 1, WindowBounds.Min.y, WindowBounds.Max.y);
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
    gs_thread_context ThreadContext = Win32CreateThreadContext();
    
    {
        m44 Before = m44{
            0, 1, 2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11,
            12, 13, 14, 15
        };
        m44 After = M44Transpose(Before);
        OutputDebugStringA("Before:\n");
        PrintMatrix(Before, ThreadContext);
        OutputDebugStringA("\n\n");
        OutputDebugStringA("After:\n");
        PrintMatrix(After, ThreadContext);
        OutputDebugStringA("\n\n");
        
    }
    
    {
        v4 Before = {1, 2, 3, 4};
        m44 Transform = {};
        for (u32 i = 0; i < 16; i++)
        {
            Transform.Array[i] = i + 1;
        }
        v4 After = Transform * Before;
        Assert(V4Mag(After - v4{30, 70, 110, 150}) < .00000001f);
    }
    
    { // Translation
        v4 Before = {0, 0, 0, 1};
        m44 Translation = M44Translation(v4{5, 5, 5, 0});
        v4 After = Translation * Before;
        Assert((After == v4{5, 5, 5, 1}));
    }
    
    { // X Rotation
        v4 Before = {0, 5, 0, 1};
        m44 Forward = M44RotationX(HalfPiR32);
        m44 Backward = M44RotationX(-HalfPiR32);
        v4 After = Forward * Before;
        Assert(V4Mag(After - v4{0, 0, -5, 1}) < .000001f);
        After = Backward * Before;
        Assert(V4Mag(After - v4{0, 0, 5, 1}) < .000001f);
    }
    
    { // Y Rotation
        v4 Before = {5, 0, 0, 1};
        m44 Forward = M44RotationY(HalfPiR32);
        m44 Backward = M44RotationY(-HalfPiR32);
        v4 After = Forward * Before;
        Assert(V4Mag(After - v4{0, 0, -5, 1}) < .000001f);
        After = Backward * Before;
        Assert(V4Mag(After - v4{0, 0, 5, 1}) < .000001f);
    }
    
    { // Z Rotation
        v4 Before = {0, 5, 0, 1};
        m44 Forward = M44RotationZ(HalfPiR32);
        m44 Backward = M44RotationZ(-HalfPiR32);
        v4 After = Forward * Before;
        Assert(V4Mag(After - v4{-5, 0, 0, 1}) < .000001f);
        After = Backward * Before;
        Assert(V4Mag(After - v4{5, 0, 0, 1}) < .000001f);
    }
    
    { // Combined X Rotation
        v4 Before = {0, 5, 0, 1};
        m44 Forward = M44Rotation(v3{HalfPiR32, 0, 0});
        m44 Backward = M44Rotation(v3{-HalfPiR32, 0, 0});
        v4 After = Forward * Before;
        Assert(V4Mag(After - v4{0, 0, -5, 1}) < .000001f);
        After = Backward * Before;
        Assert(V4Mag(After - v4{0, 0, 5, 1}) < .000001f);
    }
    
    { // Combined Y Rotation
        v4 Before = {5, 0, 0, 1};
        m44 Forward = M44Rotation(v3{0, HalfPiR32, 0});
        m44 Backward = M44Rotation(v3{0, -HalfPiR32, 0});
        v4 After = Forward * Before;
        Assert(V4Mag(After - v4{0, 0, -5, 1}) < .000001f);
        After = Backward * Before;
        Assert(V4Mag(After - v4{0, 0, 5, 1}) < .000001f);
    }
    
    { // Combined Z Rotation
        v4 Before = {0, 5, 0, 1};
        m44 Forward = M44Rotation(v3{0, 0, HalfPiR32});
        m44 Backward = M44Rotation(v3{0, 0, -HalfPiR32});
        v4 After = Forward * Before;
        Assert(V4Mag(After - v4{-5, 0, 0, 1}) < .000001f);
        After = Backward * Before;
        Assert(V4Mag(After - v4{5, 0, 0, 1}) < .000001f);
    }
    
    { // Translate then Rotate
        v4 Before = v4{0, 0, 0, 1};
        
        m44 Translation = M44Translation(v4{5, 0, 0, 0});
        m44 Rotation = M44Rotation(v3{0, 0, HalfPiR32});
        m44 Composite = Rotation * Translation;
        
        v4 Inbetween = Translation * Before;
        v4 After = Rotation * Inbetween;
        Assert(V4Mag(After - v4{0, 5, 0, 1}) < .000001f);
        
        After = Composite * Before;
        Assert(V4Mag(After - v4{0, 5, 0, 1}) < .000001f);
    }
    
    { // Two translations
        v4 Before = v4{0, 0, 0, 1};
        m44 TranslationA = M44Translation(v4{5, 0, 0, 0});
        m44 TranslationB = M44Translation(v4{0, 5, 0, 0});
        v4 After = TranslationB * TranslationA * Before;
        Assert(V4Mag(After - v4{5, 5, 0, 1}) < .000001f);
    }
    
    { // Perspective Transform
        rect2 WindowBounds = rect2{
            v2{0, 0},
            v2{1440.0f, 768.0f},
        };
        
        m44 Matrix = M44Translation(v4{0, 0, -200, 0}) * M44Rotation(v3{0, DegToRadR32(45), 0});
        m44 Projection = M44ProjectionPerspective(45, RectAspectRatio(WindowBounds), 0.1f, 500);
        
        r32 Rad = 25;
        v4 P0 = Matrix * v4{-Rad, -Rad, 0, 1};
        v4 P1 = Matrix * v4{Rad, -Rad, 0, 1};
        v4 P2 = Matrix * v4{Rad, Rad, 0, 1};
        v4 P3 = Matrix * v4{-Rad, Rad, 0, 1};
        
        v4 P0P = Projection * P0;
        v4 P1P = Projection * P1;
        v4 P2P = Projection * P2;
        v4 P3P = Projection * P3;
        
        v4 P0PD = PerspectiveDivide(P0P);
        v4 P1PD = PerspectiveDivide(P1P);
        v4 P2PD = PerspectiveDivide(P2P);
        v4 P3PD = PerspectiveDivide(P3P);
        
        v4 P0S = ToScreen(P0PD, WindowBounds);
        P0S.w = 1;
        
        v4 P1S = ToScreen(P1PD, WindowBounds);
        P1S.w = 1;
        
        v4 P2S = ToScreen(P2PD, WindowBounds);
        P2S.w = 1;
        
        v4 P3S = ToScreen(P3PD, WindowBounds);
        P3S.w = 1;
        
        Assert(V4Mag(P0S - v4{630.11401, 256.88202, 0.99930286, 1}) < 0.00001f);
        Assert(V4Mag(P1S - v4{795.28662, 277.52859, 0.99948108, 1}) < 0.00001f);
        Assert(V4Mag(P2S - v4{795.28662, 490.47144, 0.99948108, 1}) < 0.00001f);
        Assert(V4Mag(P3S - v4{630.11401, 511.11798, 0.99930286, 1}) < 0.00001f);
        
        //PushRenderQuad2D(RenderBuffer, P0S.xy, P1S.xy, P2S.xy, P3S.xy, WhiteV4);
    }
    
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
        u8* InputQueueMemory = (u8*)Win32Alloc(InputQueueMemorySize, 0);
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
    
    HANDLE WorkQueueSemaphoreHandle = CreateSemaphoreEx(0, 0, PLATFORM_THREAD_COUNT, 0, 0, SEMAPHORE_ALL_ACCESS);
    
    gs_work_queue WorkQueue = {};
    WorkQueue.SemaphoreHandle = &WorkQueueSemaphoreHandle;
    WorkQueue.JobsMax = 512;
    WorkQueue.Jobs = (gs_threaded_job*)Win32Alloc(sizeof(gs_threaded_job) * WorkQueue.JobsMax, 0);
    WorkQueue.NextJobIndex = 0;
    WorkQueue.PushWorkOnQueue = Win32PushWorkOnQueue;
    WorkQueue.CompleteQueueWork = Win32DoQueueWorkUntilDone;
    WorkQueue.ResetWorkQueue = ResetWorkQueue;
    
    for (s32 i = 0; i < PLATFORM_THREAD_COUNT; i++)
    {
        // ID = 0 is reserved for this thread
        WorkerThreads[i].Queue = &WorkQueue;
        WorkerThreads[i].Handle = CreateThread(0, 0, &WorkerThreadProc, (void*)&WorkerThreads[i], 0, 0);
    }
    
    s32 InitialMemorySize = MB(64);
    u8* InitialMemory = (u8*)Win32Alloc(InitialMemorySize, 0);
    context Context = {};
    Context.ThreadContext = ThreadContext;
    Context.MemorySize = InitialMemorySize;
    Context.MemoryBase = InitialMemory;
    Context.WindowBounds = rect2{v2{0, 0}, v2{(r32)MainWindow.Width, (r32)MainWindow.Height}};
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
    
    s32 RenderMemorySize = MB(12);
    u8* RenderMemory = (u8*)Win32Alloc(RenderMemorySize, 0);
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
        
        Context.WindowBounds = rect2{v2{0, 0}, v2{(r32)MainWindow.Width, (r32)MainWindow.Height}};
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