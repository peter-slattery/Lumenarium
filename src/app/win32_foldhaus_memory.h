//
// File: win32_foldhaus_memory.h
// Author: Peter Slattery
// Creation Date: 2020-02-04
//
//
// NOTE: Relies on having imported foldhaus_platform.h prior to this file
//
#ifndef WIN32_FOLDHAUS_MEMORY_H

ALLOCATOR_ALLOC(Win32Alloc)
{
    u8* Result = (u8*)VirtualAlloc(NULL, Size,
                                   MEM_COMMIT | MEM_RESERVE,
                                   PAGE_EXECUTE_READWRITE);
    if (ResultSize != 0)
    {
        *ResultSize = Size;
    }
    return Result;
}

ALLOCATOR_FREE(Win32Free)
{
    b32 Result = VirtualFree(Ptr, 0, MEM_RELEASE);
    if (!Result)
    {
        s32 Error = GetLastError();
        // TODO(Peter): I'm waiting to see an error actually occur here
        // to know what it could possibly be.
        InvalidCodePath;
    }
}

#define WIN32_FOLDHAUS_MEMORY_H
#endif // WIN32_FOLDHAUS_MEMORY_H