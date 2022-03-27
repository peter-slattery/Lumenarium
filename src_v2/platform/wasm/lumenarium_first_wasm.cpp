
#define WASM_EXPORT __attribute__((visibility("default")))
#define WASM_EXTERN extern "C" 

#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }

#include "../lumenarium_compiler_flags.h"
#include "../lumenarium_platform_common_includes.h"

#include "../../lumenarium_types.h"
#include "../lumenarium_platform.h"
#include "../../lumenarium_first.cpp"

#include "lumenarium_wasm_webgl.cpp"

#include "lumenarium_wasm_memory.cpp"
// window
#include "lumenarium_wasm_time.cpp"
#include "lumenarium_wasm_file.cpp"
#include "lumenarium_wasm_thread.cpp"

WASM_EXTERN void print(const char* text, int len);

typedef void wasm_animation_frame_cb(u32 time_elapsed);
WASM_EXTERN void wasm_request_animation_frame(wasm_animation_frame_cb* cb);

EXTERN_C_BEGIN;

int 
str_len (char* str)
{
  int result = 0;
  while (str[result] != 0) result++;
  return result;
}

u8* dest = 0;

App_State* wasm_app_state = 0;

WASM_EXPORT void 
update(u32 time_elapsed)
{
  lumenarium_frame_prepare(wasm_app_state);
  lumenarium_frame(wasm_app_state);
  
  // TODO(PS): check for app running flags
  wasm_request_animation_frame(update);
}

WASM_EXPORT int
main(void) 
{ 
  wasm_app_state = lumenarium_init();
  //wasm_request_animation_frame(update);
  return 0;
  
#if 0
  Platform_Ticks first = platform_get_ticks();
  
  char* str0 = "Hi there!";
  int str0_len = str_len(str0);
  print(str0, str0_len);
  
  char a = str0[0];
  char* str1 = (char*)platform_mem_reserve(32);
  char b = str0[0];
  
  for (int i = 0; i < str0_len; i++)
  {
    int it = str0_len - (i + 1);
    str1[i] = str0[i];
  }
  print(str1, str0_len);
  
  char* file = "text.txt";
  int file_len = str_len(file);
  
  dest = platform_mem_reserve(KB(4));
  wasm_fetch(file, file_len, dest, KB(4));
  
  Platform_Ticks last = platform_get_ticks();
  r64 seconds_elapsed = get_seconds_elapsed(first, last);
  
  return last.value - first.value; 
#endif
}

EXTERN_C_END;