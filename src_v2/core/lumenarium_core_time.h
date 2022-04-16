#if !defined(LUMENARIUM_CORE_TIME_H)
#define LUMENARIUM_CORE_TIME_H

typedef struct { s64 value; } Ticks;

internal Ticks get_ticks_elapsed(Ticks start, Ticks end);
internal r64   ticks_to_seconds(Ticks t, r64 ticks_per_second);
internal r64   get_seconds_elapsed(Ticks start, Ticks end, r64 ticks_per_second);

///////////////////////////////////////
///////////////////////////////////////
//    Implementation
///////////////////////////////////////
///////////////////////////////////////

internal Ticks
get_ticks_elapsed(Ticks start, Ticks end)
{
  Ticks result = {};
  result.value = end.value - start.value;
  return result;
}

internal r64
ticks_to_seconds(Ticks t, r64 ticks_per_second)
{
  r64 result = t.value / ticks_per_second;
  return result;
}

internal r64
get_seconds_elapsed(Ticks start, Ticks end, r64 ticks_per_second)
{
  Ticks diff = get_ticks_elapsed(start, end);
  return ticks_to_seconds(diff, ticks_per_second);
}


#endif // LUMENARIUM_CORE_TIME_H