/* date = March 26th 2022 3:42 pm */

#ifndef LUMENARIUM_ASSERT_H
#define LUMENARIUM_ASSERT_H

#if defined(PRINT_ASSERTS)
#  include <stdio.h>
#  define err_write(s,...) err_write_(s,__VA_ARGS__)
static FILE* file_err;
void err_write_(char* fmt, ...) {
  if (!file_err) return;
  va_list args;
  va_start(args, fmt);
  vfprintf(file_err, fmt, args);
  va_end(args);
}
void open_err_file() { file_err = fopen("./err.txt", "wb"); }
void close_err_file() { fclose(file_err); }
#else
#  define err_write(s,...)
void open_err_file() {}
void close_err_file() {}
#endif

#if !defined(PLATFORM_wasm)

// this assert works by simply trying to write to an invalid address
// (in this case, 0x0), which will crash in most debuggers
#  define assert_always (*((volatile s32*)0) = 0xFFFF)

#else
WASM_EXTERN void wasm_assert_always(char* file, unsigned int file_len, unsigned int line);
#  define assert_always wasm_assert_always(__FILE__, sizeof(__FILE__), __LINE__)
#endif // defined(PLATFORM_WASM)

#ifdef USE_ASSERTS
#  define assert(c) if (!(c)) { err_write("Assert Hit: %s:%d\n", __FILE__, (u32)__LINE__); close_err_file(); assert_always; }

// useful for catching cases that you aren't sure you'll hit, but
// want to be alerted when they happen
#  define invalid_code_path assert(0);

// useful for switch statements on enums that might grow. You'll
// break in the debugger the first time the default case is hit
// with a new enum value
#  define invalid_default_case default: { assert(0); } break;

#else
#  define assert(c)
#  define invalid_code_path
#  define invalid_default_case default: { } break;
#endif

#endif //LUMENARIUM_ASSERT_H
