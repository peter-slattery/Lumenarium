#ifndef LUMENARIUM_LINUX_TIME_H
#define LUMENARIUM_LINUX_TIME_H 1

#define NANOS_PER_SECOND 1000000000
Ticks 
os_get_ticks()
{
  struct timespec ts = {};
  s32 r = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  assert(r == 0);
  s64 nanos = (s64)ts.tv_sec * NANOS_PER_SECOND;
  nanos += (s64)ts.tv_nsec;

  Ticks result = {
    .value = nanos
  };
  return result;
}

r64   
os_get_ticks_per_second()
{
  return NANOS_PER_SECOND;
}

#endif // LUMENARIUM_LINUX_TIME_H