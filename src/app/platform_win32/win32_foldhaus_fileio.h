//
// File: win32_foldhaus_fileio.h
// Author: Peter Slattery
// Creation Date: 2020-02-04
//
//
// NOTE: Relies on having imported foldhaus_platform.h prior to this file
//
#ifndef WIN32_FOLDHAUS_FILEIO_H

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

internal void
Win32SetFileInfoFromFindFileData(gs_file_info* Info, WIN32_FIND_DATA FindFileData, gs_const_string SearchPath, gs_memory_arena* Storage)
{
    u32 FileNameLength = CharArrayLength(FindFileData.cFileName);
    
    // NOTE(Peter): String Storage
    // Storing the string in the final storage means we don't have to copy the string later, and all
    // strings will be continguous in memory at the calling site, though they will be before the array
    gs_string FileName = PushString(Storage, SearchPath.Length + FileNameLength + 1);
    PrintF(&FileName, "%S%.*s", SearchPath, FileName.Size, FindFileData.cFileName);
    NullTerminate(&FileName);
    
    Info->FileSize = Win32HighLowToU64(FindFileData.nFileSizeLow, FindFileData.nFileSizeHigh);
    Info->CreationTime = Win32FileTimeToU64(FindFileData.ftCreationTime);
    Info->LastWriteTime = Win32FileTimeToU64(FindFileData.ftLastWriteTime);
    Info->Path = FileName.ConstString;
    Info->IsDirectory = HasFlag(FindFileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
}

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
            b32 AddFile = true;
            
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
                
                AddFile = HasFlag(Flags, EnumerateDirectory_IncludeDirectories);
            }
            
            if (AddFile)
            {
                temp_file_list_entry* File = PushStruct(FileHandler.Transient, temp_file_list_entry);
                *File = {0};
                Win32SetFileInfoFromFindFileData(&File->Info, FindFileData, SearchPath, Storage);
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

#define WIN32_FOLDHAUS_FILEIO_H
#endif // WIN32_FOLDHAUS_FILEIO_H