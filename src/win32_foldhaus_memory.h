//
// File: win32_foldhaus_memory.h
// Author: Peter Slattery
// Creation Date: 2020-02-04
//
//
// NOTE: Relies on having imported foldhaus_platform.h prior to this file
//
#ifndef WIN32_FOLDHAUS_MEMORY_H

internal u8* 
Win32BasicAlloc (s32 Size) 
{ 
    return (u8*)VirtualAlloc(NULL, Size, 
                             MEM_COMMIT | MEM_RESERVE, 
                             PAGE_EXECUTE_READWRITE);
}

PLATFORM_ALLOC(Win32Alloc)
{
    u8* Result = Win32BasicAlloc(Size);
    return Result;
}

PLATFORM_FREE(Win32Free)
{
    b32 Result = VirtualFree(Base, 0, MEM_RELEASE);
    if (!Result)
    {
        s32 Error = WSAGetLastError();
        InvalidCodePath;
    }
    return Result;
}

PLATFORM_REALLOC(Win32Realloc)
{
    u8* NewMemory = Win32BasicAlloc(NewSize);
    if (Base)
    {
        GSMemCopy(Base, NewMemory, OldSize);
        Win32Free(Base, OldSize);
    }
    return NewMemory;
}

#define WIN32_FOLDHAUS_MEMORY_H
#endif // WIN32_FOLDHAUS_MEMORY_H