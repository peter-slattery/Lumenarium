
// set by calling win32_get_performance_frequency()
global s64 win32_performance_counter_freq_s64 = 0;
global r64 win32_performance_counter_freq_r64 = 0;

s64
win32_get_performance_frequency()
{
  LARGE_INTEGER freq;
  if (!QueryPerformanceFrequency(&freq))
  {
    win32_get_last_error();
    // TODO(Peter): I'm waiting to see an error actually occur here
    // to know what it could possibly be.
    invalid_code_path;
  }
  return (s64)freq.QuadPart;
}

void
win32_time_init()
{
  win32_performance_counter_freq_s64 = win32_get_performance_frequency();
  win32_performance_counter_freq_r64 = (r64)win32_performance_counter_freq_s64;
}

Platform_Ticks
platform_get_ticks()
{
  Platform_Ticks result = {};
  LARGE_INTEGER time;
  if (!QueryPerformanceCounter(&time))
  {
    win32_get_last_error();
    // TODO(Peter): I'm waiting to see an error actually occur here
    // to know what it could possibly be.
    invalid_code_path;
  }
  result.value = (s64)time.QuadPart;
  return result;
}

r64
platform_ticks_to_seconds(Platform_Ticks ticks)
{
  r64 result = (r64)ticks.value / win32_performance_counter_freq_r64;
  return result;
}