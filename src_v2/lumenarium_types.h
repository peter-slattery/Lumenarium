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

struct Data
{
  u8* base;
  u64 size;
};

#define Bytes(x) (x)
#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) (((u64)x) << 40)

#define has_flag(data, flag) (((data) & (flag)) != 0)
#define has_flag_only(data, flag) (((data) & (flag)) == (data))
#define add_flag(data, flag) ((data) |= (flag))
#define rem_flag(data, flag) ((data) &= (~(flag)))

//////////////////////////////////////////////
//         Assert

// this assert works by simply trying to write to an invalid address
// (in this case, 0x0), which will crash in most debuggers
#define assert_always (*((volatile s32*)0) = 0xFFFF)

#ifdef USE_ASSERTS
#  define assert(c) if (!(c)) { assert_always; }

// useful for catching cases that you aren't sure you'll hit, but
// want to be alerted when they happen
#  define invalid_code_path assert_always

// useful for switch statements on enums that might grow. You'll
// break in the debugger the first time the default case is hit
// with a new enum value
#  define invalid_default_case default: { assert_always; } break;

#else
#  define assert(c)
#  define invalid_code_path
#  define invalid_default_case default: { } break;
#endif

//////////////////////////////////////////////
//         List Helper Macros

#define sll_push(first,last,ele) \
if (!(first)) { (first) = (ele); }\
else { (last)->next = (ele); } \
(last) = (ele); (ele)->next = 0; 

// TODO(PS): Stack, Queue, DLL ops

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

typedef struct Allocator Allocator;

#endif //LUMENARIUM_TYPES_H
