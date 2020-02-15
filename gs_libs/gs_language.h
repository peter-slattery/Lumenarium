#ifndef GS_LANGUAGE_H

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__)
#include <intrin.h>

// TODO(Peter): Get rid of math.h
#include <math.h>

#elif defined(__APPLE__) && defined(__MAC__)
// TODO(Peter): 

#else // Std lib
#include <stdlib.h>

#endif

#define internal static
#define local_persist static
#define global_variable static


#if !defined(GS_TYPES)

#define GSINT64(s) (s) ## L
#define GSUINT64(s) (s) ## UL

typedef signed char    b8;
typedef short int      b16;
typedef int            b32;
typedef long long int  b64;

typedef unsigned char          u8;
typedef unsigned short int     u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;

typedef signed char   s8;
typedef short int     s16;
typedef int           s32;
typedef long long int s64;

typedef float  r32;
typedef double r64;

#ifndef _STDINT

#define INT8_MIN   (-128)
#define INT16_MIN  (-32767-1)
#define INT32_MIN  (-2147483647-1)
#define INT64_MIN  (-GSINT64(9223372036854775807)-1)

#define INT8_MAX   (127)
#define INT16_MAX  (32767)
#define INT32_MAX  (2147483647)
#define INT64_MAX  (GSINT64(9223372036854775807))

#define UINT8_MAX  (255)
#define UINT16_MAX (65535)
#define UINT32_MAX (4294967295U)
#define UINT64_MAX (GSUINT64(18446744073709551615))

#endif // _STDINT

#define FLOAT_MIN  (1.175494351e-38F)
#define FLOAT_MAX  (3.402823466e+38F)
#define DOUBLE_MIN (2.2250738585072014e-308)
#define DOUBLE_MAX (1.7976931348623158e+308)

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)
#define Terabytes(Value) (Gigabytes(Value) * 1024)

#ifndef PI
#define PI  3.14159265359
#endif

#define TAU 6.2831853071
#define PI_OVER_180 0.01745329251f

#define GS_TYPES
#endif


#ifdef DEBUG

static void DebugPrint(char* Format, ...);

#if !defined(Assert)
// NOTE(peter): this writes to address 0 which is always illegal and will cause a crash
#define Assert(expression) \
if((expression)) \
{ \
}else{ \
volatile int* p = 0; \
*p = 5; \
}
#endif

#define DEBUG_IF(condition) if (condition)

#define InvalidCodePath Assert(0)
#define InvalidDefaultCase default: { Assert(0); }
#define DebugBreak __debugbreak()

#define STBI_ASSERT(x) Assert(x)

#ifdef GS_TEST_SUTE
#define TestClean(v, c) SuccessCount += Test(v, c, &TestCount)
internal s32
Test(b32 Result, char* Description, s32* Count)
{
    char* Passed = (Result ? "Success" : "Failed");
    if (!Result)
        DebugPrint("%s:\n................................................%s\n\n", Description, Passed);
    
    *Count = *Count + 1;
    return (Result ? 1 : 0);
}
#endif // GS_TEST_SUTE

#ifndef GS_LANGUAGE_NO_PROFILER_DEFINES
#ifndef DEBUG_TRACK_SCOPE
#define DEBUG_TRACK_SCOPE(a)
#endif // DEBUG_TRACK_SCOPE
#ifndef DEBUG_TRACK_FUNCTION
#define DEBUG_TRACK_FUNCTION
#endif // DEBUG_TRACK_FUNCTION
#endif // GS_LANGUAGE_NO_PROFILER_DEFINES

#else

#define Assert(expression)
#define InvalidCodePath
#define DEBUG_IF(condition)

#ifndef GS_LANGUAGE_NO_PROFILER_DEFINES
#ifndef DEBUG_TRACK_SCOPE
#define DEBUG_TRACK_SCOPE(a)
#endif // DEBUG_TRACK_SCOPE
#ifndef DEBUG_TRACK_FUNCTION
#define DEBUG_TRACK_FUNCTION
#endif // DEBUG_TRACK_FUNCTION
#endif // GS_LANGUAGE_NO_PROFILER_DEFINES

#endif // DEBUG

#ifndef GS_LANGUAGE_MATH

#define GSArrayLength(arr) (sizeof(arr) / sizeof(arr[0]))

#define GSZeroStruct(data) GSZeroMemory_((u8*)(&(data)), sizeof(data))
#define GSZeroMemory(mem, size) GSZeroMemory_((u8*)(mem), (size))
#define GSZeroArray(arr, type, count) GSZeroMemory_((u8*)(arr), (sizeof(type) * count))
static void
GSZeroMemory_ (u8* Memory, s32 Size)
{
    for (int i = 0; i < Size; i++) { Memory[i] = 0; }
}

#define GSMemCopy(from, to, size) GSMemCopy_((u8*)from, (u8*)to, size)
static void
GSMemCopy_ (u8* From, u8* To, s32 Size)
{
    for (int i = 0; i < Size; i++) { To[i] = From[i]; }
}

#define GSMemSet(buffer, value, size) GSMemSet_((u8*)buffer, value, size)
internal void
GSMemSet_ (u8* Buffer, u8 Value, s32 Length)
{
    u8* Cursor = Buffer;
    for (s32 i = 0; i < Length; i++)
    {
        *Cursor++ = Value;
    }
}

#define GSMinDef(type) static type GSMin(type A, type B) { return (A < B ? A : B); }
GSMinDef(s8)
GSMinDef(s16)
GSMinDef(s32)
GSMinDef(s64)
GSMinDef(u8)
GSMinDef(u16)
GSMinDef(u32)
GSMinDef(u64)
GSMinDef(r32)
GSMinDef(r64)
#undef GSMinDef

#define GSMaxDef(type) static type GSMax(type A, type B) { return (A > B ? A : B); }
GSMaxDef(s8)
GSMaxDef(s16)
GSMaxDef(s32)
GSMaxDef(s64)
GSMaxDef(u8)
GSMaxDef(u16)
GSMaxDef(u32)
GSMaxDef(u64)
GSMaxDef(r32)
GSMaxDef(r64)
#undef GSMaxDef

inline b32 XOR(b32 A, b32 B)
{
    b32 Result = (A == !B);
    return Result;
}
#define GSClampDef(type) static type GSClamp(type Min, type V, type Max) { \
type Result = V; \
if (V < Min) { Result = Min; } \
if (V > Max) { Result = Max; } \
return Result; \
}
GSClampDef(s8)
GSClampDef(s16)
GSClampDef(s32)
GSClampDef(s64)
GSClampDef(u8)
GSClampDef(u16)
GSClampDef(u32)
GSClampDef(u64)
GSClampDef(r32)
GSClampDef(r64)
#undef GSClampDef

#define GSClamp01Def(type) static type GSClamp01(type V) { \
type Min = 0; type Max = 1; \
type Result = V; \
if (V < Min) { Result = Min; } \
if (V > Max) { Result = Max; } \
return Result; \
}
GSClamp01Def(r32)
GSClamp01Def(r64)
#undef GSClamp01Def

#define GSAbsDef(type) static type GSAbs(type A) { return (A < 0 ? -A : A); }
GSAbsDef(s8)
GSAbsDef(s16)
GSAbsDef(s32)
GSAbsDef(s64)
GSAbsDef(r32)
GSAbsDef(r64)
#undef GSAbsDef

#define GSPowDef(type) static type GSPow(type N, s32 Power) { \
type Result = N; \
for(s32 i = 1; i < Power; i++) { Result *= N; } \
return Result; \
}
GSPowDef(s8)
GSPowDef(s16)
GSPowDef(s32)
GSPowDef(s64)
GSPowDef(u8)
GSPowDef(u16)
GSPowDef(u32)
GSPowDef(u64)
GSPowDef(r32)
GSPowDef(r64)
#undef GSPowDef


#define GSLerpDef(type) type GSLerp(type A, type B, type Percent) { return (A * (1.0f - Percent))+(B * Percent);}
GSLerpDef(r32)
GSLerpDef(r64)
#undef GSLerpDef

static r32 GSSqrt(r32 V) 
{ 
    r32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(V)));
    return Result; 
}
#if 0
// TODO(Peter): Need a way to split the input into two f32's to supply to _mm_sqrt_sd
static r64 GSSqrt(r64 V) 
{ 
    r64 Result = _mm_cvtsd_f64(_mm_sqrt_sd(_mm_set_sd(V)));
    return Result;
}
#endif

static r32 DegreesToRadians (r32 Degrees) { return Degrees * PI_OVER_180; }
static r64 DegreesToRadians (r64 Degrees) { return Degrees * PI_OVER_180; }

#define GSIsPowerOfTwoDef(type) static type IsPowerOfTwo(type V) { return (V & (V - 1)) == 0; }
GSIsPowerOfTwoDef(u8);
GSIsPowerOfTwoDef(u16);
GSIsPowerOfTwoDef(u32);
GSIsPowerOfTwoDef(u64);
#undef GSIsPowerOfTwoDef

#define GSIsOddDef(type) inline type IsOdd(type V) { return (V & 1); }
GSIsOddDef(u8);
GSIsOddDef(u16);
GSIsOddDef(u32);
GSIsOddDef(u64);
GSIsOddDef(s8);
GSIsOddDef(s16);
GSIsOddDef(s32);
GSIsOddDef(s64);
#undef GSIsOddDef

#define GSIntDivideRoundUpDef(type) static type IntegerDivideRoundUp (type A, type B) { r32 Result = (r32)A / (r32)B; Result += .99999f; return (type)Result; }
GSIntDivideRoundUpDef(u8);
GSIntDivideRoundUpDef(u16);
GSIntDivideRoundUpDef(u32);
GSIntDivideRoundUpDef(u64);
GSIntDivideRoundUpDef(s8);
GSIntDivideRoundUpDef(s16);
GSIntDivideRoundUpDef(s32);
GSIntDivideRoundUpDef(s64);
#undef GSIntDivideRoundUpDef

#define GSRemapDef(type) \
static type GSRemap(type Value, type OldMin, type OldMax, type NewMin, type NewMax) { \
    type Result = (Value - OldMin) / (OldMax - OldMin); \
    Result = (Result * (NewMax - NewMin)) + NewMin; \
    return Result; \
}
GSRemapDef(u8);
GSRemapDef(u16);
GSRemapDef(u32);
GSRemapDef(u64);
GSRemapDef(s8);
GSRemapDef(s16);
GSRemapDef(s32);
GSRemapDef(s64);
GSRemapDef(r32);
GSRemapDef(r64);
#undef GSRemapDef

static r32
GSFloor(r32 Value)
{
    return floor(Value);
}

static r32
GSFract(r32 Value)
{
    return Value - GSFloor(Value);
}

static r32
GSModF(r32 Value, r32 Int)
{
    r32 Div = Value / Int;
    r32 Fract = GSAbs(GSFract(Div));
    return Int * Fract;
}

#define GSTrigFunctionDef(name, type, func) static type name(type V) { return func(V); }
GSTrigFunctionDef(GSSin, r32, sinf);
GSTrigFunctionDef(GSSin, r64, sin);
GSTrigFunctionDef(GSCos, r32, cosf);
GSTrigFunctionDef(GSCos, r64, cos);
GSTrigFunctionDef(GSTan, r32, tanf);
GSTrigFunctionDef(GSTan, r64, tan);
#undef GSTrigFunctionDef

static u8
RoundToNearestPowerOfTwo (u8 V)
{
    u8 Result = 0;
    
    if (IsPowerOfTwo(V)) 
    { 
        Result = V; 
    }
    else
    {
        Result  = V - 1;
        Result |= Result >> 1;
        Result |= Result >> 2;
        Result |= Result >> 4;
        Result += 1;
    }
    
    return Result;
}

static u16
RoundToNearestPowerOfTwo (u16 V)
{
    u16 Result = 0;
    
    if (IsPowerOfTwo(V)) 
    { 
        Result = V; 
    }
    else
    {
        Result  = V - 1;
        Result |= Result >> 1;
        Result |= Result >> 2;
        Result |= Result >> 4;
        Result |= Result >> 8;
        Result += 1;
    }
    
    return Result;
}

static u32
RoundToNearestPowerOfTwo (u32 V)
{
    u32 Result = 0;
    
    if (IsPowerOfTwo(V)) 
    { 
        Result = V; 
    }
    else
    {
        Result  = V - 1;
        Result |= Result >> 1;
        Result |= Result >> 2;
        Result |= Result >> 4;
        Result |= Result >> 8;
        Result |= Result >> 16;
        Result += 1;
    }
    
    return Result;
}

static u64
RoundToNearestPowerOfTwo (u64 V)
{
    u64 Result = 0;
    
    if (IsPowerOfTwo(V)) 
    { 
        Result = V; 
    }
    else
    {
        Result = V - 1;
        Result |= Result >> 1;
        Result |= Result >> 2;
        Result |= Result >> 4;
        Result |= Result >> 8;
        Result |= Result >> 16;
        Result |= Result >> 32;
        Result += 1;
    }
    
    return Result;
}

#define GS_LANGUAGE_MATH
#endif // GS_LANGUAGE_MATH

static u32
HostToNetU32(u32 In)
{
    unsigned char *s = (unsigned char *)&In;
    u32 Result = (u32)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
    return Result;
}

static u16
HostToNetU16(u16 In)
{
    unsigned char *s = (unsigned char *)&In;
    u16 Result = (u16)(s[0] << 8 | s[1]);
    return Result;
}

#define GS_LANGUAGE_H
#endif