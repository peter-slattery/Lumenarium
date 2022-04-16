/* date = April 11th 2022 6:11 pm */

#ifndef LUMENARIUM_RANDOM_H
#define LUMENARIUM_RANDOM_H

typedef struct Random_Series Random_Series;
struct Random_Series
{
  u32 last_value;
};

internal Random_Series
random_series_create(u32 seed)
{
  Random_Series result = {};
  result.last_value = seed;
  return result;
}

internal u32
random_series_next(Random_Series* s)
{
  u32 result = s->last_value;
  result ^= result << 13;
  result ^= result >> 17;
  result ^= result << 5;
  s->last_value = result;
  return result;
}

internal r32
random_series_next_unilateral(Random_Series* s)
{
  r32 result = random_series_next(s) / (r32)(0xFFFFFFFF);
  return result;
}

internal r32
random_series_next_bilateral(Random_Series* s)
{
  r32 result = random_series_next(s) / (r32)(0xFFFFFFFF);
  result = (result * 2.0f) - 1.0f;
  return result;
}



#endif //LUMENARIUM_RANDOM_H
