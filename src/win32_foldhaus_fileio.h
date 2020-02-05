//
// File: win32_foldhaus_fileio.h
// Author: Peter Slattery
// Creation Date: 2020-02-04
//
//
// NOTE: Relies on having imported foldhaus_platform.h prior to this file
//
#ifndef WIN32_FOLDHAUS_FILEIO_H

PLATFORM_READ_ENTIRE_FILE(Win32ReadEntireFile)
{
    platform_memory_result Result = {};
    Result.Error = PlatformMemory_NoError;
    
    HANDLE FileHandle = CreateFileA (Path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD FileSize = GetFileSize(FileHandle, NULL);
        Result.Base = (u8*)VirtualAlloc(NULL, FileSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (Result.Base) 
        {
            Result.Size = FileSize;
            
            s32 BytesRead = 0;
            if (ReadFile(FileHandle, (LPVOID)Result.Base, FileSize, (LPDWORD)(&BytesRead), NULL))
            {
                
            }
            else
            {
                u32 Error = GetLastError();
                // TODO(Peter): 
                Result.Size = 0;
                Result.Error = PlatformMemory_UnknownError;
            }
        }
        CloseHandle(FileHandle);
    }
    else
    {
        Result.Error = PlatformMemory_FileNotFound;
    }
    
    return Result;
}

PLATFORM_WRITE_ENTIRE_FILE(Win32WriteEntireFile)
{
    b32 Result = false;
    HANDLE FileHandle = CreateFileA (
                                     Path,
                                     GENERIC_WRITE,
                                     0,
                                     NULL,
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL);
    
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten = 0;
        
        b32 WriteSuccess = WriteFile(FileHandle,
                                     Contents, Size,
                                     &BytesWritten,
                                     NULL);
        
        if (WriteSuccess && BytesWritten == (u32)Size)
        {
            CloseHandle(FileHandle);
            Result = true;
        }
        else
        {
            Result = false;
        }
    }
    else
    {
        Result = false;
    }
    
    return Result;
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
        // TODO(Peter): Error handling
    }
    
    return Result;
}

PLATFORM_GET_FILE_PATH(Win32SystemDialogueOpenFile)
{
    b32 Result = false;
    
    PathBuffer[0] = 0;
    
    OPENFILENAMEA OpenFileName = {};
    OpenFileName.lStructSize = sizeof(OpenFileName);
    OpenFileName.hwndOwner = NULL;
    OpenFileName.lpstrFilter = FilterStrings;
    OpenFileName.lpstrCustomFilter = NULL; // NOTE(Peter): for preserving last filter string chosen
    OpenFileName.nMaxCustFilter = 0; // NOTE(Peter): ignored since we left CustomFilter null
    OpenFileName.nFilterIndex = 1;
    OpenFileName.lpstrFile = PathBuffer;
    OpenFileName.nMaxFile = BufferLength;
    OpenFileName.lpstrFileTitle = NULL;
    OpenFileName.nMaxFileTitle = 0; // NOTE(Peter): Ignored since fileTitle is null
    OpenFileName.lpstrInitialDir = NULL;
    OpenFileName.lpstrTitle = NULL;
    OpenFileName.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST;
    OpenFileName.lpstrDefExt = NULL;
    
    Result = GetOpenFileNameA (&OpenFileName);
    
    return Result;
}

internal directory_listing
EnumerateDirectory(char* Path, memory_arena* Storage)
{
    directory_listing Result = {};
    // TODO(Peter): 
    return Result;
}

#define WIN32_FOLDHAUS_FILEIO_H
#endif // WIN32_FOLDHAUS_FILEIO_H