//
// File: win32_foldhaus_timing.h
// Author: Peter Slattery
// Creation Date: 2020-02-04
//
//
// NOTE: Relies on having imported foldhaus_platform.h prior to this file
//
#ifndef WIN32_FOLDHAUS_TIMING_H

internal s64
GetPerformanceFrequency ()
{
  LARGE_INTEGER Frequency;
  if (!QueryPerformanceFrequency(&Frequency))
  {
    s32 Error = GetLastError();
    // TODO(Peter): I'm waiting to see an error actually occur here
    // to know what it could possibly be.
    InvalidCodePath;
  }
  return (s64)Frequency.QuadPart;
}

internal s64
GetWallClock ()
{
#if 0
  s64 Result = __rdtsc();
  return Result;
#else
  LARGE_INTEGER Time;
  if (!QueryPerformanceCounter(&Time))
  {
    s32 Error = GetLastError();
    // TODO(Peter): I'm waiting to see an error actually occur here
    // to know what it could possibly be.
    InvalidCodePath;
  }
  return (s64)Time.QuadPart;
#endif
}


#define WIN32_FOLDHAUS_TIMING_H
#endif // WIN32_FOLDHAUS_TIMING_H