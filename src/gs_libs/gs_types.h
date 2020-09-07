//
// File: gs_types.h
// Author: Peter Slattery
// Creation Date: 2020-04-18
//
#ifndef GS_TYPES_H

#if defined(__clang__)
# pragma GCC diagnostic ignored "-Wunused-value"
# pragma GCC diagnostic ignored "-Wvarargs"
# pragma GCC diagnostic ignored "-Wwritable-strings"
#endif

#if defined(_MSC_VER)
# include <intrin.h>
#endif

// Someday, we home to remove these includes
#include <stdarg.h>
#if !defined(GUESS_INTS)
# include <stdint.h>
#endif // !defined(GUESS_INTS)

#include <math.h>

#define Glue_(a,b) a##b
#define Glue(a,b) Glue_(a,b)

#define Stringify_(a) #a
#define Stringify(a) Stringify_(a)

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

enum gs_basic_type
{
    gs_BasicType_char,
    gs_BasicType_b8,
    gs_BasicType_b32,
    gs_BasicType_b64,
    gs_BasicType_u8,
    gs_BasicType_u16,
    gs_BasicType_u32,
    gs_BasicType_u64,
    gs_BasicType_s8,
    gs_BasicType_s16,
    gs_BasicType_s32,
    gs_BasicType_s64,
    gs_BasicType_r32,
    gs_BasicType_r64,
    
    gs_BasicType_Count,
};

global_const u64 gs_BasicTypeSizes[] =
{
    sizeof(char),
    sizeof(b8),
    sizeof(b32),
    sizeof(b64),
    sizeof(u8),
    sizeof(u16),
    sizeof(u32),
    sizeof(u64),
    sizeof(s8),
    sizeof(s16),
    sizeof(s32),
    sizeof(s64),
    sizeof(r32),
    sizeof(r64),
};

internal u64
BasicTypeSize(gs_basic_type Type)
{
    return gs_BasicTypeSizes[(u32)Type];
}

global_const u8  MaxU8  = 0xFF;
global_const u16 MaxU16 = 0xFFFF;
global_const u32 MaxU32 = 0xFFFFFFFF;
global_const u64 MaxU64 = 0xFFFFFFFFFFFFFFFF;

global_const s8  MaxS8  = 127;
global_const s16 MaxS16 = 32767;
global_const s32 MaxS32 = 2147483647;
global_const s64 MaxS64 = 9223372036854775807;

global_const s8  MinS8  = -127 - 1;
global_const s16 MinS16 = -32767 - 1;
global_const s32 MinS32 = -2147483647 - 1;
global_const s64 MinS64 = -9223372036854775807 - 1;

global_const r32 MaxR32  = 3.402823466e+38f;
global_const r32 MinR32  = -MaxR32;
global_const r32 SmallestPositiveR32 = 1.1754943508e-38f;
global_const r32 EpsilonR32 = 5.96046448e-8f;

global_const r32 PiR32 = 3.14159265359f;
global_const r32 HalfPiR32 = 1.5707963267f;
global_const r32 TauR32 = 6.28318530717f;

global_const r64 MaxR64 = 1.79769313486231e+308;
global_const r64 MinR64 = -MaxR64;
global_const r64 SmallestPositiveR64 = 4.94065645841247e-324;
global_const r64 EpsilonR64 = 1.11022302462515650e-16;

// TODO: va_start and va_arg replacements

internal r32
DegToRadR32(r32 Degrees)
{
    return (Degrees * (PiR32 / 180.0f));
}

internal r32
RadToDegR32(r32 Radians)
{
    return (Radians * (180.0f / PiR32));
}

struct s8_array
{
    s8* Values;
    u32 Count;
    u32 CountMax;
};

struct s16_array
{
    s16* Values;
    u32 Count;
    u32 CountMax;
};

struct s32_array
{
    s32* Values;
    u32 Count;
    u32 CountMax;
};

struct s64_array
{
    s64* Values;
    u32 Count;
    u32 CountMax;
};

struct u8_array
{
    u8* Values;
    u32 Count;
    u32 CountMax;
};

struct u16_array
{
    u16* Values;
    u32 Count;
    u32 CountMax;
};

struct u32_array
{
    u32* Values;
    u32 Count;
    u32 CountMax;
};

struct u64_array
{
    u64* Values;
    u32 Count;
    u32 CountMax;
};


#define PointerDifference(a,b) ((u8*)(a) - (u8*)(b))
#define PointerToInt(a) PointerDifference(a, 0)
#define Member(type,member) (((type*)0)->member)
#define MemberOffset(type,member) PointerToInt(&Member(type,member))

// NOTE(Peter): Makes sure that s doesn't expand in some way that invalidates
// a nested if statement.
#define Statement(s) do{ s }while(0)

//
// Asserts
//
// AssertAlways and AssertMessageAlways should be used sparingly, because they'll
// still assert in the final build
#define AssertBreak(m) (*((volatile s32*)0) = 0xFFFF)
#define AssertAlways(c) Statement( if (!(c)) { AssertBreak(c); } )
#define AssertMessageAlways(m) AssertBreak(m)

#if !SHIP_MODE
# define Assert(c) AssertAlways(c)
# define AssertMessage(m) AssertBreak(m)
# define InvalidDefaultCase default: { AssertBreak("invalid default case"); } break;
# define StaticAssert(c) \
enum { \
Glue(gs_AssertFail_, __LINE__) = 1 / (int)(!!(c)), \
}
#else
# define Assert(c)
# define AssertMessage(m)
# define InvalidDefaultCase default: {} break;
# define StaticAssert(c)
#endif

#define AssertImplies(a,b) Statement(if(a) { Assert(b); })
#define InvalidCodePath AssertMessage("invalid code path")
#define NotImplemented AssertMessage("not implemented")
#define DontCompile ImAfraidICantDoThat

#define LineNumberString Stringify(__LINE__)
#define FileNameAndLineNumberString_ __FILE__ ":" LineNumberString ":"
#define FileNameAndLineNumberString (char*)FileNameAndLineNumberString_

//

#define Bytes(x) (x)
#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) (((u64)x) << 40)

#define HasFlag(data, flag) (((data) & (flag)) != 0)
#define HasFlagOnly(data, flag) (((data) & (flag)) == (data))
#define AddFlag(data, flag) ((data) |= (flag))
#define RemoveFlag(data, flag) ((data) &= (~(flag)))

#define Max(a,b) (((a) > (b)) ? (a) : (b))
#define Min(a,b) (((a) > (b)) ? (b) : (a))
#define Clamp_(a,x,b) ((x < a) ? a : ((x > b) ? b : x))
#define Clamp(a,x,b) Clamp_((a), (x), (b))
#define Clamp01(x) Clamp_(0.0f, (x), 1.0f)
#define Abs(x) (((x) < 0) ? ((x) * -1) : x)
#define Sign(x) ((x) < 0) ? -1 : 1;
#define IsPowerOfTwo(x) (((x) & ((x) - 1)) == 0)
#define IsOdd(x) (((x) & 1) != 0)

internal void
ZeroMemory_(u8* Memory, u64 Size)
{
    for (u64 i = 0; i < Size; i++)
    {
        Memory[i] = 0;
    }
}

internal void
CopyMemory_(u8* From, u8* To, u64 Size)
{
    for (u64 i = 0; i < Size; i++)
    {
        To[i] = From[i];
    }
}

#define StaticArrayLength(arr) sizeof(arr) / sizeof((arr)[0])
#define ZeroMemoryBlock(mem,size) ZeroMemory_((u8*)(mem), (size))
#define ZeroStruct(str) ZeroMemory_((str), sizeof(str))
#define ZeroArray(arr, type, count) ZeroMemory_((u8*)(arr), sizeof(type) * (count))

#define CopyArray(from, to, type, count) CopyMemory_((u8*)(from), (u8*)(to), sizeof(type) * (count))
#define CopyMemoryTo(from, to, size) CopyMemory_((u8*)(from), (u8*)(to), (size))
// Singly Linked List Utilities

#define SLLPush_(list_tail,new_ele) list_tail->Next = new_ele, list_tail = new_ele
#define SLLPush(list_tail,new_ele) (SLLPush_((list_tail), (new_ele)))

#define SLLPop_(list_tail) list_tail=list_tail=list_tail->next
#define SLLPop(list_tail) (SLLPop_((list_tail)))

#define SLLNext(ele_at) ele_at = ele_at->Next;
#define SLLPrev(ele_at) ele_at = ele_at->Prev;

#define SLLInit(head,tail,first_ele) head=first_ele, tail=first_ele;

#define SLLPushOrInit(first,last,new_ele) \
if (last) { SLLPush(last, new_ele); } \
else { SLLInit(first,last,new_ele); }

// Vectors

union v2
{
    struct
    {
        r32 x;
        r32 y;
    };
    r32 E[2];
};

union v3
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
    };
    struct
    {
        r32 r;
        r32 g;
        r32 b;
    };
    struct
    {
        v2 xy;
        r32 _z0;
    };
    struct
    {
        r32 _x0;
        v2 yz;
    };
    r32 E[3];
};

union v4
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
        r32 w;
    };
    struct
    {
        r32 r;
        r32 g;
        r32 b;
        r32 a;
    };
    struct
    {
        v2 xy;
        v2 zw;
    };
    struct
    {
        r32 _x0;
        v2 yz;
        r32 _w0;
    };
    struct
    {
        v3 xyz;
        r32 _w1;
    };
    r32 E[4];
};

struct v4_ray
{
    v4 Origin;
    v4 Direction;
};

#define WhiteV4 v4{1, 1, 1, 1}
#define BlackV4 v4{0, 0, 0, 1}
#define RedV4 v4{1, 0, 0, 1}
#define GreenV4 v4{0, 1, 0, 1}
#define BlueV4 v4{0, 0, 1, 1}
#define YellowV4 v4{1, 1, 0, 1}
#define TealV4 v4{0, 1, 1, 1}
#define PinkV4 v4{1, 0, 1, 1}

#define V2Expand(v) v.x, v.y
#define V3Expand(v) v.x, v.y, v.z
#define V4Expand(v) v.x, v.y, v.z, v.w

struct v2_array
{
    v2* Vectors;
    u32 Count;
    u32 CountMax;
};

struct v3_array
{
    v3* Vectors;
    u32 Count;
    u32 CountMax;
};

struct v4_array
{
    v4* Vectors;
    u32 Count;
    u32 CountMax;
};

struct range1
{
    r32 Min;
    r32 Max;
};

struct range2
{
    v2 Min;
    v2 Max;
};
typedef range2 rect2;

struct range3
{
    v3 Min;
    v3 Max;
};

struct range4
{
    v4 Min;
    v4 Max;
};

struct range1_array
{
    range1* Ranges;
    u32 Count;
    u32 CountMax;
};

struct range2_array
{
    range2* Ranges;
    u32 Count;
    u32 CountMax;
};

struct range3_array
{
    range3* Ranges;
    u32 Count;
    u32 CountMax;
};

struct range4_array
{
    range4* Ranges;
    u32 Count;
    u32 CountMax;
};

#define Range1Expand(r) (r).Min, (r).Max
#define Range2Expand(r) (r).Min, (r).Max
#define Rect2Expand(r) (r).Min (r).Max
#define Range3Expand(r) (r).Min, (r).Max
#define Range4Expand(r) (r).Min, (r).Max

// Matrices
// NOTE(Peter): All matrices are stored in row major order

union m33
{
    float Array[9];
    struct
    {
        r32 AXx; r32 AYx; r32 AZx;
        r32 AXy; r32 AYy; r32 AZy;
        r32 AXz; r32 AYz; r32 AZz;
    };
};

union m44
{
    float Array[16];
    struct
    {
        r32 AXx; r32 AYx; r32 AZx; r32 Tx;
        r32 AXy; r32 AYy; r32 AZy; r32 Ty;
        r32 AXz; r32 AYz; r32 AZz; r32 Tz;
        r32 AXw; r32 AYw; r32 AZw; r32 Tw;
    };
};

struct m33_array
{
    m33* Matrices;
    u32 Count;
    u32 CountMax;
};

struct m44_array
{
    m44* Matrices;
    u32 Count;
    u32 CountMax;
};

//////////////////////////
//
// Strings

struct gs_const_string
{
    union
    {
        char* Str;
        u8* Data;;
    };
    u64 Length;
};

struct gs_string
{
    union
    {
        gs_const_string ConstString;
        struct
        {
            char* Str;
            u64 Length;
        };
    };
    u64 Size;
};

struct gs_const_string_array
{
    gs_const_string* Strings;
    u64 Count;
    u64 Used;
};

struct gs_string_array
{
    gs_string* Strings;
    u64 Count;
    u64 CountMax;
};

internal u64
CStringLength(char* Str)
{
    char* At = Str;
    while (*At) { At++; }
    return PointerDifference(At, Str);
}

#define StringExpand(str) (int)(str).Length, (str).Str
#define LitString(cstr) gs_const_string{(char*)(cstr), CStringLength((char*)cstr) }

// The index of the character in these arrays corresponds to its value as a number in
// the relevant base, so you can do FindFirst on them with a char to get the int value
//   ie. 3 is at index 3 in Base10Chars.
//   ie. C is at index 12 in Base16Chars.
global_const gs_const_string Base8Chars  = LitString("01234567");
global_const gs_const_string Base10Chars = LitString("0123456789");
global_const gs_const_string Base16Chars = LitString("0123456789ABCDEF");

//////////////////////////
//
// Thread Context

typedef struct gs_thread_context gs_thread_context;

//////////////////////////
//
// Memory

struct gs_data
{
    u8* Memory;
    u64 Size;
};

struct gs_data_array
{
    gs_data* Data;
    u64 Count;
    u64 CountMax;
};

enum gs_access_flag
{
    gs_AccessFlag_Read  = 1 << 0,
    gs_AccessFlag_Write = 1 << 1,
    gs_AccessFlag_Exec  = 1 << 2,
};

typedef s32 gs_scan_direction;
enum
{
    gs_Scan_Backward = -1,
    gs_Scan_Forward  = 1,
};

#define ALLOCATOR_ALLOC(name) void* name(u64 Size, u64* ResultSize)
typedef ALLOCATOR_ALLOC(allocator_allocate);

#define ALLOCATOR_FREE(name) void name(void* Ptr, u64 Size)
typedef ALLOCATOR_FREE(allocator_free);

struct gs_allocator
{
    allocator_allocate* Alloc;
    allocator_free* Free;
};

struct gs_memory_cursor
{
    gs_data Data;
    u64 Position;
};

struct gs_memory_cursor_list
{
    gs_memory_cursor  Cursor;
    gs_memory_cursor_list* Next;
    gs_memory_cursor_list* Prev;
};

enum arena_type
{
    Arena_BaseArena,
    Arena_SubArena,
};

struct gs_memory_arena
{
    arena_type Type;
    gs_allocator Allocator;
    gs_memory_arena* Parent;
    
    gs_memory_cursor_list* CursorList;
    u64 MemoryChunkSize;
    u64 MemoryAlignment;
};

struct gs_memory_arena_array
{
    gs_memory_arena* Arenas;
    u32 Count;
    u32 Size;
};

///////////////////////////////
//
// String Builder
//

struct gs_string_builder_buffer
{
    gs_string String;
    gs_string_builder_buffer* Next;
};

struct gs_string_builder
{
    gs_memory_arena* Arena;
    u32 BufferSize;
    gs_string_builder_buffer* Root;
    gs_string_builder_buffer* Head;
};

///////////////////////////////
//
// Debug Output
//

typedef struct debug_output debug_output;

#define DEBUG_PRINT(name) void name(debug_output Output, gs_const_string Message)
typedef DEBUG_PRINT(debug_print);

struct debug_output
{
    gs_memory_arena* Storage;
    debug_print* Print;
};

///////////////////////////////
//
// Dynamic Array
//
// I like having constant lookup times, along with growable arrays. :)
// NOTE(Peter): If you're using this, you probably want to write a Get<element_type>
// procedure to auto cast the result to the type you want. I'm even providing a
// debug check for you to make sure that you're requesting a size that matches
// the ElementSize for extra safety.

struct gs_dynarray_buffer
{
    u8* Memory;
};

struct gs_dynarray
{
    gs_memory_arena Arena;
    
    gs_dynarray_buffer* Buffers;
    u64 BuffersCount;
    u64 ElementCount;
    
    u64 ElementSize;
    u64 ElementsPerBuffer;
};

struct gs_dynarray_handle
{
    u64 BufferIndex;
    u64 IndexInBuffer;
};

#define INVALID_DYNARRAY_HANDLE gs_dynarray_handle{0, 0}

struct gs_dynarray_handle_list
{
    gs_dynarray_handle* Handles;
    u32 Count;
    u32 Size;
};

///////////////////////////////
//
// File IO

// TODO(Peter): Error Handling Thought
// The gs_file_handler, gs_allocator, etc should contain pointers to a central error buffer
// where errors can be logged

enum enumerate_directory_flag
{
    EnumerateDirectory_Recurse = 1 << 0,
    EnumerateDirectory_IncludeDirectories = 1 << 1,
};

struct gs_file_info
{
    gs_const_string Path;
    gs_const_string AbsolutePath;
    u64 FileSize;
    u64 CreationTime;
    u64 LastWriteTime;
    b32 IsDirectory;
};

struct gs_file_info_array
{
    gs_file_info* Values;
    u32 Count;
    u32 MaxCount;
};

struct gs_file
{
    union
    {
        gs_data Data;
        struct
        {
            u8* Memory;
            u64 Size;
        };
    };
    gs_file_info FileInfo;
};

typedef struct gs_file_handler gs_file_handler;

#define GET_FILE_INFO(name) gs_file_info name(gs_file_handler FileHandler, gs_const_string Path)
typedef GET_FILE_INFO(file_handler_get_file_info);

#define READ_ENTIRE_FILE(name) gs_file name(gs_file_handler FileHandler, gs_const_string Path, gs_data Memory)
typedef READ_ENTIRE_FILE(file_handler_read_entire_file);

#define WRITE_ENTIRE_FILE(name) bool  name(gs_file_handler FileHandler, gs_const_string Path, gs_data Data)
typedef WRITE_ENTIRE_FILE(file_handler_write_entire_file);

#define ENUMERATE_DIRECTORY(name) gs_file_info_array  name(gs_file_handler FileHandler, gs_memory_arena* Storage, gs_const_string Path, u32 Flags)
typedef ENUMERATE_DIRECTORY(file_handler_enumerate_directory);

struct gs_file_handler
{
    file_handler_get_file_info* GetFileInfo;
    file_handler_read_entire_file* ReadEntireFile;
    file_handler_write_entire_file* WriteEntireFile;
    file_handler_enumerate_directory* EnumerateDirectory;
    gs_memory_arena* Transient;
};


//////////////////////////
//
// Timing

#define GET_WALL_CLOCK(name) s64 name()
typedef GET_WALL_CLOCK(get_wall_clock);

#define GET_SECONDS_ELAPSED(name) r64 name(u64 StartCycles, u64 EndCycles)
typedef GET_SECONDS_ELAPSED(get_seconds_elapsed);

struct gs_time_handler
{
    get_wall_clock* GetWallClock;
    get_seconds_elapsed* GetSecondsElapsed;
};

///////////////////////////////
//
// Random

struct gs_random_series
{
    u32 Value;
};

///////////////////////////////
//
// Sort

struct gs_radix_list
{
    u64_array Radixes;
    u64_array IDs;
};


///////////////////////////////
//
// Mouse/Keyboard Input

enum gs_event_type
{
    gs_EventType_Unknown,
    
    // Reached end of event stream
    gs_EventType_NoMoreEvents,
    // There was an event but it requires no action from the using program
    gs_EventType_NoEvent,
    
    gs_EventType_KeyPressed,
    gs_EventType_KeyReleased,
    
    gs_EventType_MouseMoved,
    gs_EventType_MouseWheel,
    
    gs_EventType_Count,
};

enum gs_key
{
    gs_Key_Invalid,
    
    gs_Key_Esc,
    
    gs_Key_Space,
    gs_Key_Tab,
    gs_Key_CapsLock,
    gs_Key_Shift, gs_Key_LeftShift, gs_Key_RightShift,
    gs_Key_Control, gs_Key_LeftCtrl, gs_Key_RightCtrl,
    gs_Key_Fn,
    gs_Key_Alt,
    gs_Key_PageUp, gs_Key_PageDown,
    gs_Key_End, gs_Key_Home, gs_Key_Select,
    gs_Key_Backspace, gs_Key_Delete,
    gs_Key_Enter,
    
    // Function Keys
    gs_Key_F0, gs_Key_F1, gs_Key_F2, gs_Key_F3, gs_Key_F4, gs_Key_F5, gs_Key_F6, gs_Key_F7,
    gs_Key_F8, gs_Key_F9, gs_Key_F10, gs_Key_F11, gs_Key_F12,
    
    // Letters
    gs_Key_a, gs_Key_b, gs_Key_c, gs_Key_d, gs_Key_e, gs_Key_f, gs_Key_g, gs_Key_h,
    gs_Key_i, gs_Key_j, gs_Key_k, gs_Key_l, gs_Key_m, gs_Key_n, gs_Key_o, gs_Key_p,
    gs_Key_q, gs_Key_r, gs_Key_s, gs_Key_t, gs_Key_u, gs_Key_v, gs_Key_w, gs_Key_x,
    gs_Key_y, gs_Key_z,
    
    gs_Key_A, gs_Key_B, gs_Key_C, gs_Key_D, gs_Key_E, gs_Key_F, gs_Key_G, gs_Key_H,
    gs_Key_I, gs_Key_J, gs_Key_K, gs_Key_L, gs_Key_M, gs_Key_N, gs_Key_O, gs_Key_P,
    gs_Key_Q, gs_Key_R, gs_Key_S, gs_Key_T, gs_Key_U, gs_Key_V, gs_Key_W, gs_Key_X,
    gs_Key_Y, gs_Key_Z,
    
    // Numbers
    gs_Key_0, gs_Key_1, gs_Key_2, gs_Key_3, gs_Key_4, gs_Key_5, gs_Key_6, gs_Key_7,
    gs_Key_8, gs_Key_9,
    
    gs_Key_Num0, gs_Key_Num1, gs_Key_Num2, gs_Key_Num3, gs_Key_Num4, gs_Key_Num5,
    gs_Key_Num6, gs_Key_Num7, gs_Key_Num8, gs_Key_Num9,
    
    // Symbols
    gs_Key_Bang, gs_Key_At, gs_Key_Pound, gs_Key_Dollar, gs_Key_Percent, gs_Key_Carrot,
    gs_Key_Ampersand, gs_Key_Star, gs_Key_LeftParen, gs_Key_RightParen, gs_Key_Minus, gs_Key_Plus,
    gs_Key_Equals, gs_Key_Underscore, gs_Key_OpenSquareBracket, gs_Key_CloseSquareBracket, gs_Key_OpenCurlyBracket,
    gs_Key_CloseCurlyBracket, gs_Key_Colon, gs_Key_SemiColon, gs_Key_SingleQuote, gs_Key_DoubleQuote,
    gs_Key_ForwardSlash, gs_Key_Backslash, gs_Key_Pipe, gs_Key_Comma, gs_Key_Period,
    gs_Key_QuestionMark, gs_Key_LessThan, gs_Key_GreaterThan, gs_Key_Tilde, gs_Key_BackQuote,
    
    // Arrows
    gs_Key_UpArrow,
    gs_Key_DownArrow,
    gs_Key_LeftArrow,
    gs_Key_RightArrow,
    
    // Mouse
    // NOTE(Peter): Including this here so we can utilize the same KeyDown, KeyUp etc. functions
    gs_Key_MouseLeftButton,
    gs_Key_MouseMiddleButton,
    gs_Key_MouseRightButton,
    gs_Key_MouseX1Button,
    gs_Key_MouseX2Button,
    
    gs_Key_Count,
};

enum gs_modifier_key_flags
{
    gs_ModifierKeyFlag_Shift = 1 << 0,
    gs_ModifierKeyFlag_Ctrl = 1 << 1,
    gs_ModifierKeyFlag_Alt = 1 << 2,
};

struct gs_input_event
{
    gs_event_type Type;
    gs_key Key;
    v2 Position;
    r32 Amount;
    b32 Modifiers;
};

struct gs_input_event_buffer
{
    gs_input_event* Values;
    u32 Count;
    u32 MaxCount;
};

struct gs_mouse_state
{
    v2 Position;
    b32 ButtonStates[3];
    v2 DownPosition;
};

#define MouseButton_IsDownBit 0
#define MouseButton_WasDownBit 1
#define MouseButton_IsDown 1
#define MouseButton_IsNotDown 0
#define MouseButton_WasDown 1
#define MouseButton_WasNotDown 0

//////////////////////////
//
// Thread Context

struct gs_thread_info
{
    u32 ThreadID;
};

struct gs_thread_context
{
    gs_thread_info ThreadInfo;
    
    // TODO(Peter): Pull these handlers out into just a gs_context struct so
    // they can be shared across threads.
    // specifically the allocator
    gs_allocator Allocator;
    gs_file_handler FileHandler;
    debug_output DebugOutput;
    gs_time_handler TimeHandler;
    
    gs_memory_arena* Transient;
};

// Threads & Work Queue

typedef struct gs_work_queue gs_work_queue;

struct gs_worker_thread
{
    gs_thread_context Context;
    gs_work_queue* Queue;
    b32 ShouldExit;
};

#define THREAD_PROC(name) void name(gs_thread_context Context, gs_data Data)
typedef THREAD_PROC(thread_proc);

struct gs_threaded_job
{
    thread_proc* WorkProc;
    gs_data Data;
    gs_const_string JobName;
};

#define PUSH_WORK_ON_QUEUE(name) void name(gs_work_queue* Queue, thread_proc* WorkProc, gs_data Data, gs_const_string JobName)
typedef PUSH_WORK_ON_QUEUE(push_work_on_queue);

#define COMPLETE_QUEUE_WORK(name) void name(gs_work_queue* Queue, gs_thread_context Context)
typedef COMPLETE_QUEUE_WORK(complete_queue_work);

#define RESET_WORK_QUEUE(name) void name(gs_work_queue* Queue)
typedef RESET_WORK_QUEUE(reset_work_queue);

struct gs_work_queue
{
    void* SemaphoreHandle;
    
    u32 JobsMax;
    u32 volatile JobsCount;
    u32 volatile NextJobIndex;
    u32 volatile JobsCompleted;
    gs_threaded_job* Jobs;
    
    // Work Queue
    push_work_on_queue* PushWorkOnQueue;
    complete_queue_work* CompleteQueueWork;
    reset_work_queue* ResetWorkQueue;
};

#define GS_TYPES_H
#endif // GS_TYPES_H