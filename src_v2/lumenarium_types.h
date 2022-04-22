// TODO(PS) @DEPRECATE - new os layer

/* date = March 22nd 2022 2:08 am */

#ifndef LUMENARIUM_TYPES_H
#define LUMENARIUM_TYPES_H

#define internal static
#define local_persist static
#define global static
#define local_const static const
#define global_const static const
#define external extern "C"

#if defined(GUESS_INTS)
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#else
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

typedef s8 b8;
typedef s32 b32;
typedef s64 b64;

typedef float r32;
typedef double r64;

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


#define get_byte(value, byte_index) ((value >> (8 * byte_index)) & 0xFF)

struct Data
{
  u8* base;
  u64 size;
};

internal Data
data_create(u8* base, u64 size)
{
  Data result = {};
  result.base = base;
  result.size = size;
  return result;
}

#define memory_zero_array(b,t,c) memory_zero((u8*)(b), sizeof(t) * (c))
internal void memory_zero(u8* base, u64 size);
internal void memory_copy(u8* from, u8* to, u64 size);

//////////////////////////////////////////////
//         Math

#ifndef max
#  define max(a,b) (a) > (b) ? (a) : (b)
#endif

#ifndef min
#  define min(a,b) (a) > (b) ? (b) : (a)
#endif

#define lerp(a,t,b) (a) + ((1.0f - (t)) * (b))
#define clamp(r0,v,r1) min((r1),max((r0),(v)))
#define lerp_clamp(a,t,b) clamp((a),lerp((a),(t),(b)),(b))

//////////////////////////////////////////////
//         String

// NOTE(PS): even though this has a len and cap, it should always be
// null terminated
struct String
{
  u8* str;
  u64 len;
  u64 cap;
};

internal String string_create(u8* str, u64 len, u64 cap);
internal u64 string_copy_to(String* dest, String src);

//////////////////////////////////////////////
//         Data Writer

struct Data_Writer
{
  Data data;
  u64 at;
};

// NOTE(PS): functions ending in _b treat data in the Data_Writer as big endian 
// order (network ordering) where functions ending in _l treat data into little 
// endian order
// It is always assumed that values not in the Data_Writer (ie the other args
// to the function or the functions return value) are in little endian order

internal void 
dw_put_u8(Data_Writer* w, u8 b)
{
  if (w->at < w->data.size)
  {
    w->data.base[w->at++] = b;
  }
}

internal u8
dw_get_u8(Data_Writer* w)
{
  u8 result = 0;
  if (w->at < w->data.size)
  {
    result = w->data.base[w->at];
  }
  return result;
}

internal void
dw_put_u16_b(Data_Writer* w, u16 b)
{
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 0));
}

internal void
dw_put_u16_l(Data_Writer* w, u16 b)
{
  dw_put_u8(w, get_byte(b, 0));
  dw_put_u8(w, get_byte(b, 1));
}

internal void
dw_put_u32_b(Data_Writer* w, u32 b)
{
  dw_put_u8(w, get_byte(b, 3));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 0));
}

internal void
dw_put_u32_l(Data_Writer* w, u32 b)
{
  dw_put_u8(w, get_byte(b, 0));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 3));
}

internal void
dw_put_u64_b(Data_Writer* w, u64 b)
{
  dw_put_u8(w, get_byte(b, 7));
  dw_put_u8(w, get_byte(b, 6));
  dw_put_u8(w, get_byte(b, 5));
  dw_put_u8(w, get_byte(b, 4));
  dw_put_u8(w, get_byte(b, 3));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 0));
}

internal void
dw_put_u64_l(Data_Writer* w, u64 b)
{
  dw_put_u8(w, get_byte(b, 0));
  dw_put_u8(w, get_byte(b, 1));
  dw_put_u8(w, get_byte(b, 2));
  dw_put_u8(w, get_byte(b, 3));
  dw_put_u8(w, get_byte(b, 4));
  dw_put_u8(w, get_byte(b, 5));
  dw_put_u8(w, get_byte(b, 6));
  dw_put_u8(w, get_byte(b, 7));
}

internal void
dw_put_str(Data_Writer* w, String str)
{
  for (u64 i = 0; i < str.len; i++)
  {
    dw_put_u8(w, str.str[i]);
  }
}

internal void
dw_put_str_min_len(Data_Writer* w, String str, u64 min_len)
{
  u64 start = w->at;
  dw_put_str(w, str);
  if (str->len < min_len)
  {
    w->at = start + min_len;
  }
}

internal void
dw_put_str_min_len_nullterm(Data_Writer* w, String str, u64 min_len)
{
  dw_put_str_min_len(w, str, min_len);
  w->data.base[w->at - 1] = '\0';
}

// TODO(PS): get functions

#define Bytes(x) (x)
#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) (((u64)x) << 30)
#define TB(x) (((u64)x) << 40)

#define has_flag(data, flag) (((data) & (flag)) != 0)
#define has_flag_only(data, flag) (((data) & (flag)) == (data))
#define add_flag(data, flag) ((data) |= (flag))
#define rem_flag(data, flag) ((data) &= (~(flag)))

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
//         Math

internal u32
round_up_to_pow2(u32 v)
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
//         Vector Extensions

#define v2_to_v3(xy,z) v3{(xy).x, (xy).y, (z)}
#define v2_floor(v) v2{ floorf(v.x), floorf(v.y) }
#define v3_floor(v) v3{ floorf(v.x), floorf(v.y), floorf(v.z) }

internal bool
rect2_contains(v2 min, v2 max, v2 point)
{
  return (
          min.x <= point.x && min.y <= point.y &&
          max.x >= point.x && max.y >= point.y
          );
}

//////////////////////////////////////////////
//         Color Constants

#define WHITE_V4 v4{1,1,1,1}
#define BLACK_V4 v4{0,0,0,1}
#define RED_V4 v4{1,0,0,1}
#define GREEN_V4 v4{0,1,0,1}
#define BLUE_V4 v4{0,0,1,1}
#define YELLOW_V4 v4{1,1,0,1}
#define TEAL_V4 v4{0,1,1,1}
#define PINK_V4 v4{1,0,1,1}

typedef struct Allocator Allocator;


#endif //LUMENARIUM_TYPES_H
