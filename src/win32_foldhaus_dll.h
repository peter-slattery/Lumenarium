//
// File: win32_foldhaus_dll.h
// Author: Peter Slattery
// Creation Date: 2020-02-04
//
//
// NOTE: Relies on having imported foldhaus_platform.h prior to this file
//
#ifndef WIN32_FOLDHAUS_DLL_H

// DLL
struct win32_dll_refresh
{
    FILETIME LastWriteTime;
    HMODULE DLL;
    
    b32 IsValid;
    
    char SourceDLLPath[MAX_PATH];
    char WorkingDLLPath[MAX_PATH];
    char LockFilePath[MAX_PATH];
};

internal void
GetApplicationPath(system_path* Result)
{
    Assert(Result->Path);
    Result->PathLength = GetModuleFileNameA(0, Result->Path, Result->PathLength);
    
    u32 CharactersScanned = 0;
    u32 IndexOfLastSlash = 0;
    char *Scan = Result->Path;
    while(*Scan)
    {
        if (*Scan == '\\')
        {
            Result->IndexOfLastSlash = CharactersScanned + 1;
        }
        Scan++;
        CharactersScanned++;
    }
}

internal b32 
LoadApplicationDLL(char* DLLName,  win32_dll_refresh* DLLResult)
{
    b32 Success = false;
    Assert(DLLResult->DLL == 0);
    
    DLLResult->DLL = LoadLibraryA(DLLName); // TODO(Peter): Error checking
    if (DLLResult->DLL)
    {
        Success = true;
        DLLResult->IsValid = true;
    }
    
    return Success;
}

internal void 
UnloadApplicationDLL(win32_dll_refresh* DLL)
{
    if (DLL->DLL)
    {
        FreeLibrary(DLL->DLL);
    }
    DLL->DLL = 0;
    DLL->IsValid = false;
}

internal win32_dll_refresh
InitializeDLLHotReloading(char* SourceDLLName, 
                          char* WorkingDLLFileName, 
                          char* LockFileName)
{
    win32_dll_refresh Result = {};
    Result.IsValid = false;
    
    system_path ExePath = {};
    ExePath.PathLength = MAX_PATH;
    ExePath.Path = (char*)VirtualAlloc(NULL, ExePath.PathLength, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    GetApplicationPath(&ExePath);
    
    Win32ConcatStrings(ExePath.IndexOfLastSlash, ExePath.Path,
                       Win32StringLength(SourceDLLName), SourceDLLName,
                       MAX_PATH, Result.SourceDLLPath);
    Win32ConcatStrings(ExePath.IndexOfLastSlash, ExePath.Path,
                       Win32StringLength(WorkingDLLFileName), WorkingDLLFileName,
                       MAX_PATH, Result.WorkingDLLPath);
    Win32ConcatStrings(ExePath.IndexOfLastSlash, ExePath.Path,
                       Win32StringLength(LockFileName), LockFileName,
                       MAX_PATH, Result.LockFilePath);
    
    Win32Free((u8*)ExePath.Path, ExePath.PathLength);
    return Result;
    
}

internal b32 
HotLoadDLL(win32_dll_refresh* DLL)
{
    b32 DidReload = false;
    
    FILETIME UpdatedLastWriteTime = {};
    WIN32_FIND_DATA FindData = {};
    HANDLE FileHandle = FindFirstFileA(DLL->SourceDLLPath, &FindData);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        UpdatedLastWriteTime = FindData.ftLastWriteTime;
        FindClose(FileHandle);
    }
    
    if (CompareFileTime(&UpdatedLastWriteTime, &DLL->LastWriteTime))
    {
        WIN32_FILE_ATTRIBUTE_DATA Ignored;
        if (!GetFileAttributesEx(DLL->LockFilePath, GetFileExInfoStandard, &Ignored))
        {
            UnloadApplicationDLL(DLL);
            CopyFileA(DLL->SourceDLLPath, DLL->WorkingDLLPath, FALSE);
            LoadApplicationDLL(DLL->WorkingDLLPath, DLL);
            DLL->LastWriteTime = UpdatedLastWriteTime;
            DidReload = true;
        }
    }
    
    return DidReload;
}


#define WIN32_FOLDHAUS_DLL_H
#endif // WIN32_FOLDHAUS_DLL_H