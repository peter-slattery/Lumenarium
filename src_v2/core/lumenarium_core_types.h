#ifndef LUMENARIUM_CORE_TYPES_H
#define LUMENARIUM_CORE_TYPES_H

//////////////////////////////////////////////
// Explicit Names for static keyword
#define internal static
#define local_persist static
#define global static
#define local_const static const
#define global_const static const
#define external extern "C"

#define STMT(x) do {(x);}while(false)

//////////////////////////////////////////////
// Integer Sizing
#if defined(GUESS_INTS)

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#else // !defined(GUESS_INTS)

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#endif // defined(GUESS_INTS)

//////////////////////////////////////////////
// Basic Types

typedef s8 b8;
typedef s32 b32;
typedef s64 b64;

typedef float r32;
typedef double r64;

//////////////////////////////////////////////
// Basic Type Constants

#define u8_max  0xFF
#define u16_max 0xFFFF
#define u32_max 0xFFFFFFFF
#define u64_max 0xFFFFFFFFFFFFFFFF

#define s8_max  127
#define s16_max 32767
#define s32_max 2147483647
#define s64_max 9223372036854775807

#define s8_min  -127 - 1
#define s16_min -32767 - 1
#define s32_min -2147483647 - 1
#define s64_min -9223372036854775807 - 1

#define r32_max               3.402823466e+38f
#define r32_min               -3.402823466e+38f
#define r32_smallest_positive 1.1754943508e-38f
#define r32_epsilon           5.96046448e-8f
#define r32_pi                3.14159265359f
#define r32_half_pi           1.5707963267f
#define r32_tau               6.28318530717f

#define r64_max               1.79769313486231e+308
#define r64_min               -1.79769313486231e+308
#define r64_smallest_positive 4.94065645841247e-324
#define r64_epsilon           1.11022302462515650e-16
#define r64_pi                3.14159265359
#define r64_half_pi           1.5707963267
#define r64_tau               6.28318530717

#define NanosToSeconds 1 / 10000000.0
#define SecondsToNanos 10000000.0

//////////////////////////////////////////////
//         Math

#ifndef max
#  define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#  define min(a,b) ((a) > (b) ? (b) : (a))
#endif

#define lerp(a,t,b) (a) + ((1.0f - (t)) * (b))
#define clamp(r0,v,r1) min((r1),max((r0),(v)))
#define lerp_clamp(a,t,b) clamp((a),lerp((a),(t),(b)),(b))

internal u32
round_up_to_pow2_u32(u32 v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

//////////////////////////////////////////////
//         Flags

#define has_flag(data, flag) (((data) & (flag)) != 0)
#define has_flag_exact(data, flag) (((data) & (flag)) == (flag))
#define add_flag(data, flag) STMT((data) |= (flag))
#define rem_flag(data, flag) STMT((data) &= (~(flag)))

//////////////////////////////////////////////
//         List Helper Macros

#define sll_push(first,last,ele) \
if (!(first)) { (first) = (ele); }\
else { (last)->next = (ele); } \
(last) = (ele); (ele)->next = 0; 

// TODO(PS): Stack, Queue, DLL ops

//////////////////////////////////////////////
//         Hash Table
//
// Rather than define a data structure, to allow the most flexibility,
// this is just a set of functions that can be integrated into other 
// routines.
// In general, they expect you to track a u32* of ids and a u32 capacity

internal void
hash_table_init(u32* ids, u32 cap)
{
  for (u32 i = 0; i < cap; i++) ids[i] = 0;
}

internal u32
hash_table_find_(u32* ids, u32 cap, u32 start_id, u32 target_value)
{
  u32 index = start_id % cap;
  u32 start = index;
  do {
    if (ids[index] == target_value) break;
    index = (index + 1) % cap;
  } while (index != start);
  return index;
}

internal u32 
hash_table_register(u32* ids, u32 cap, u32 new_id)
{
  u32 index = hash_table_find_(ids, cap, new_id, 0);
  if (ids[index] != 0) return cap;
  ids[index] = new_id;
  return index;
}

internal u32
hash_table_find(u32* ids, u32 cap, u32 value)
{
  u32 result = hash_table_find_(ids, cap, value, value);
  if (ids[result] != value) return cap;
  return result;
}

//////////////////////////////////////////////
//         Predeclaring Memory Functions

u64 round_size_to_page_multiple(u64 size, u64 page_size);
u64 round_size_to_os_page_multiple(u64 size);

//////////////////////////////////////////////
//         Vector Extensions

#if defined(HANDMADE_MATH_IMPLEMENTATION)

#define v2_to_v3(xy,z) (v3){(xy).x, (xy).y, (z)}
#define v2_floor(v) (v2){ floorf(v.x), floorf(v.y) }
#define v3_floor(v) (v3){ floorf(v.x), floorf(v.y), floorf(v.z) }

internal bool
rect2_contains(v2 min, v2 max, v2 point)
{
  return (
          min.x <= point.x && min.y <= point.y &&
          max.x >= point.x && max.y >= point.y
          );
}

#endif // defined(HANDMADE_MATH_IMPLEMENTATION)

//////////////////////////////////////////////
//         Color Constants

#define WHITE_V4 (v4){1,1,1,1}
#define BLACK_V4 (v4){0,0,0,1}
#define RED_V4 (v4){1,0,0,1}
#define GREEN_V4 (v4){0,1,0,1}
#define BLUE_V4 (v4){0,0,1,1}
#define YELLOW_V4 (v4){1,1,0,1}
#define TEAL_V4 (v4){0,1,1,1}
#define PINK_V4 (v4){1,0,1,1}

#endif