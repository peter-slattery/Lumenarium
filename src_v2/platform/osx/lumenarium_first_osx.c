#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/gl.h>

#include "lumenarium_osx_memory.h"
#include "../../core/lumenarium_core.h"
#include "../lumenarium_os.h"
#include "../../lumenarium_first.c"

#undef internal
#undef external

#include <errno.h>
#include <sys/syslimits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach/mach_time.h>

#include "../../libs/glfw_osx/include/GLFW/glfw3.h"

#define osx_err_print(sub_proc) osx_err_print_((char*)__FUNCTION__, (char*)(sub_proc), errno)
void 
osx_err_print_(char* proc, char* sub_proc, s32 errsv)
{
  printf("Error: %s:%s - %d\n\t%s\n\n", proc, sub_proc, errsv, strerror(errsv));
}

#define OS_FILE_HANDLE_TYPE s32
#define OS_FILE_MAX_PATH PATH_MAX
#define OS_FILE_INVALID_HANDLE -1
#include "../shared/lumenarium_shared_file_tracker.h"
#include "../shared/lumenarium_shared_file_async_work_on_job.h"
#include "lumenarium_osx_file.h"
#include "lumenarium_osx_time.h"
#include "lumenarium_osx_graphics.h"

void osx_tests()
{
  Ticks t0 = os_get_ticks();

  // File Tests
  File_Handle file = os_file_open(lit_str("text.txt"), FileAccess_Read | FileAccess_Write, FileCreate_OpenAlways);
  File_Info info = os_file_get_info(file, global_scratch_);
  Data d = os_file_read_all(file, global_scratch_);
  os_file_write_all(file, d);
  os_file_close(file);

  // Path tests
  String path_exe = os_get_exe_path(global_scratch_);
  printf("%.*s\n", str_varg(path_exe));
  String path = string_chop_last_slash(path_exe);
  String path0 = string_copy(path, global_scratch_);
  os_pwd_set(path0);

  Ticks t1 = os_get_ticks();
  Ticks td = get_ticks_elapsed(t0, t1);
  r64 sd = ticks_to_seconds(td, os_get_ticks_per_second());
}

void 
glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

global u8* app_state_data = 0;

global Key_Code glfw_key_translation_table[] = {

};

Key_Code
osx_translate_key(int glfw_key)
{
  // TODO: turn this into an actual key_code
  return (Key_Code)glfw_key;
}

Key_Code
osx_translate_mouse_button(int glfw_button)
{
  switch (glfw_button)
  {
    case GLFW_MOUSE_BUTTON_LEFT: return KeyCode_MouseLeftButton; break;
    case GLFW_MOUSE_BUTTON_RIGHT: return KeyCode_MouseRightButton; break;
    case GLFW_MOUSE_BUTTON_MIDDLE: return KeyCode_MouseMiddleButton; break;
    invalid_default_case;
  }
  return 0;
}

void
button_event(Key_Code key, int action, int mods)
{
  Window_Event evt = {
    .kind = WindowEvent_ButtonDown,
    .key_code = key,
  };

  if (has_flag(mods, GLFW_MOD_SHIFT)) add_flag(evt.key_flags, KeyFlag_Mod_Shift);
  if (has_flag(mods, GLFW_MOD_CONTROL)) add_flag(evt.key_flags, KeyFlag_Mod_Shift);
  if (has_flag(mods, GLFW_MOD_ALT)) add_flag(evt.key_flags, KeyFlag_Mod_Shift);

  switch (action)
  {
    case GLFW_PRESS: { evt.key_flags = KeyFlag_State_IsDown; } break;
    case GLFW_REPEAT: { 
      evt.key_flags = KeyFlag_State_IsDown | KeyFlag_State_WasDown; 
    } break;
    case GLFW_RELEASE: { 
      evt.key_flags = KeyFlag_State_WasDown;
    } break;
    invalid_default_case;
  }
  lumenarium_event(evt, (App_State*)app_state_data);
}

void 
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  Key_Code kc = osx_translate_key(key);
  button_event(kc, action, mods);
}

void 
cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  Window_Event evt = {
    .kind = WindowEvent_MouseMoved,
    .mouse_x = (u32)xpos,
    .mouse_y = (u32)ypos,
  };
  lumenarium_event(evt, (App_State*)app_state_data);
}

void 
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  Key_Code kc = osx_translate_mouse_button(button);
  button_event(kc, action, mods);
}

void 
scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  Window_Event evt = {};
  evt.kind = WindowEvent_MouseScroll;
  evt.scroll_amt = xoffset;
  lumenarium_event(evt, (App_State*)app_state_data);
}

int main (int arg_count, char** args)
{
  // osx_tests();
  
  if (!glfwInit())
  {
    printf("Error: Could not initialize glfw.\n");
    return 1;
  }
  glfwSetErrorCallback(glfw_error_callback);

  glfwWindowHint(GLFW_DOUBLEBUFFER, true);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  GLFWwindow* window = glfwCreateWindow(1400, 700, "Lumenarium", NULL, NULL);
  if (!window)
  {
    printf("Error: Unable to create a glfw window\n");
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  gl = osx_load_opengl_ext();

  // Input Callbacks
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetScrollCallback(window, scroll_callback);

  App_State* state = lumenarium_init();
  app_state_data = (u8*)state;

  if (has_flag(state->flags, AppState_RunEditor))
  {
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    state->editor->content_scale = (v2){ xscale, yscale };
  }

  bool running = true;
  r64 target_seconds_per_frame = 1.0 / 30.0f;
  Ticks ticks_start = os_get_ticks();
  while(!glfwWindowShouldClose(window) && running && has_flag(state->flags, AppState_IsRunning)) {

    if (has_flag(state->flags, AppState_RunEditor))
    {
      s32 w, h;
      glfwGetWindowSize(window, &w, &h);
      state->editor->window_dim = (v2){ (r32)w, (r32)h };
    }
    
    lumenarium_frame_prepare(state);
    lumenarium_frame(state);
    lumenarium_env_validate();

    glfwSwapBuffers(window);
    glfwPollEvents();

    Ticks ticks_end = os_get_ticks();
    r64 seconds_elapsed = get_seconds_elapsed(ticks_start, ticks_end, os_get_ticks_per_second());
    while (seconds_elapsed < target_seconds_per_frame)
    {
      u32 sleep_time = (u32)(1000.0f * (target_seconds_per_frame - seconds_elapsed));
      usleep(sleep_time);
      
      ticks_end = os_get_ticks();
      seconds_elapsed = get_seconds_elapsed(ticks_start, ticks_end, os_get_ticks_per_second());
    }
    ticks_start = ticks_end;
  }

  lumenarium_cleanup(state);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}