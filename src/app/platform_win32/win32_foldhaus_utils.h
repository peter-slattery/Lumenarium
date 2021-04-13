//
// File: win32_foldhaus_utils.h
// Author: Peter Slattery
// Creation Date: 2020-10-01
//
#ifndef WIN32_FOLDHAUS_UTILS_H

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
    Log_Error(GlobalLogBuffer, "%s Line %d: Win32 Error %d\n\0", File, Line, Error);
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


#define WIN32_FOLDHAUS_UTILS_H
#endif // WIN32_FOLDHAUS_UTILS_H