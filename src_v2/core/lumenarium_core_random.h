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
random_from_u32(u32 input)
{
  u32 result = input;
  result ^= result << 13;
  result ^= result >> 17;
  result ^= result << 5;
  return result;
}

internal r32
random_unilateral_from_u32(u32 input)
{
  r32 result = random_from_u32(input) / (r32)(0xFFFFFFFF);
  return result;
}

internal u32
random_series_next(Random_Series* s)
{
  u32 result = random_from_u32(s->last_value);
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
