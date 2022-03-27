/* date = March 26th 2022 3:42 pm */

#ifndef LUMENARIUM_ASSERT_H
#define LUMENARIUM_ASSERT_H

#if !defined(PLATFORM_wasm)
// this assert works by simply trying to write to an invalid address
// (in this case, 0x0), which will crash in most debuggers
#  define assert_always (*((volatile s32*)0) = 0xFFFF)

#else
WASM_EXTERN void wasm_assert_always(char* file, u32 file_len, u32 line);
#  define assert_always wasm_assert_always(__FILE__, sizeof(__FILE__), __LINE__)
#endif // defined(PLATFORM_WASM)

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

#endif //LUMENARIUM_ASSERT_H
