/* date = May 10th 2021 11:48 pm */

#ifndef GS_MEMORY_WIN32_H
#define GS_MEMORY_WIN32_H

PLATFORM_ALLOC(Win32Alloc)
{
  u8* Result = (u8*)VirtualAlloc(NULL, Size,
                                 MEM_COMMIT | MEM_RESERVE,
                                 PAGE_EXECUTE_READWRITE);
  if (ResultSize) *ResultSize = Size;
  return Result;
}

PLATFORM_FREE(Win32Free)
{
  VirtualFree(Base, 0, MEM_RELEASE);
}

internal gs_allocator 
CreatePlatformAllocator()
{
  return AllocatorCreate(Win32Alloc, Win32Free, 0);
}

#endif //GS_MEMORY_WIN32_H
