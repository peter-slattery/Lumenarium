
// I think this SO post might be helpful
// https://stackoverflow.com/questions/30575995/multi-platform-equivalent-to-queryperformancecounter

Ticks
os_get_ticks()
{
  Ticks result = {
    .value = mach_absolute_time()
  };
  return result;
}

global mach_timebase_info_data_t osx_timebase_info;

r64
os_get_ticks_per_second()
{
  if (osx_timebase_info.denom == 0) {    
    mach_timebase_info(&osx_timebase_info);
  }
  r64 to_secs = ((r64)osx_timebase_info.denom * 1e9) / (r64)osx_timebase_info.numer;
  return to_secs;
}