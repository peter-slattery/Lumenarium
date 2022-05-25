#ifndef LUMENARIUM_LINUX_TIME_H
#define LUMENARIUM_LINUX_TIME_H 1

Ticks 
os_get_ticks()
{
  invalid_code_path;
  return (Ticks){};
}

r64   
os_get_ticks_per_second()
{
  invalid_code_path;
  return 0;
}

#endif // LUMENARIUM_LINUX_TIME_H