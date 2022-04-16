
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

r64
os_get_ticks_per_second()
{
  mach_timebase_info_data_t info;
  mach_timebase_info(&info);
  r64 to_nanos = (r64)info.numer / (r64)info.denom;
  r64 to_secs = to_nanos / 10e9;
  return to_secs;
}