#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <OpenGL/gl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "lumenarium_osx_memory.h"
#include "../../core/lumenarium_core.h"
#include "../lumenarium_os.h"

#define PLATFORM_SUPPORTS_EDITOR 1
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
#include <libkern/OSAtomic.h>
#include <pthread.h>

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
#define OS_SOCKET_TYPE s32
#define OS_SOCKET_INVALID_HANDLE -1
#include "../shared/lumenarium_shared_file_tracker.h"
#include "../shared/lumenarium_shared_file_async_work_on_job.h"
#include "../shared/lumenarium_shared_network.h"
#include "lumenarium_osx_file.h"
#include "lumenarium_osx_time.h"
#include "lumenarium_osx_graphics.h"
#include "lumenarium_osx_network.h"
#include "lumenarium_osx_thread.h"

void 
glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

global u8* app_state_data = 0;

global Key_Code glfw_key_translation_table[] = {
  [GLFW_KEY_SPACE] = KeyCode_Space,
  [GLFW_KEY_APOSTROPHE] = KeyCode_SingleQuote,
  [GLFW_KEY_COMMA] = KeyCode_Comma,
  [GLFW_KEY_MINUS] = KeyCode_Minus,
  [GLFW_KEY_PERIOD] = KeyCode_Period,
  [GLFW_KEY_SLASH] = KeyCode_ForwardSlash,
  [GLFW_KEY_0] = KeyCode_0,
  [GLFW_KEY_1] = KeyCode_1,
  [GLFW_KEY_2] = KeyCode_2,
  [GLFW_KEY_3] = KeyCode_3,
  [GLFW_KEY_4] = KeyCode_4,
  [GLFW_KEY_5] = KeyCode_5,
  [GLFW_KEY_6] = KeyCode_6,
  [GLFW_KEY_7] = KeyCode_7,
  [GLFW_KEY_8] = KeyCode_8,
  [GLFW_KEY_9] = KeyCode_9,
  [GLFW_KEY_SEMICOLON] = KeyCode_SemiColon,
  [GLFW_KEY_EQUAL] = KeyCode_Equals,
  [GLFW_KEY_A] = KeyCode_A,
  [GLFW_KEY_B] = KeyCode_B,
  [GLFW_KEY_C] = KeyCode_C,
  [GLFW_KEY_D] = KeyCode_D,
  [GLFW_KEY_E] = KeyCode_E,
  [GLFW_KEY_F] = KeyCode_F,
  [GLFW_KEY_G] = KeyCode_G,
  [GLFW_KEY_H] = KeyCode_H,
  [GLFW_KEY_I] = KeyCode_I,
  [GLFW_KEY_J] = KeyCode_J,
  [GLFW_KEY_K] = KeyCode_K,
  [GLFW_KEY_L] = KeyCode_L,
  [GLFW_KEY_M] = KeyCode_M,
  [GLFW_KEY_N] = KeyCode_N,
  [GLFW_KEY_O] = KeyCode_O,
  [GLFW_KEY_P] = KeyCode_P,
  [GLFW_KEY_Q] = KeyCode_Q,
  [GLFW_KEY_R] = KeyCode_R,
  [GLFW_KEY_S] = KeyCode_S,
  [GLFW_KEY_T] = KeyCode_T,
  [GLFW_KEY_U] = KeyCode_U,
  [GLFW_KEY_V] = KeyCode_V,
  [GLFW_KEY_W] = KeyCode_W,
  [GLFW_KEY_X] = KeyCode_X,
  [GLFW_KEY_Y] = KeyCode_Y,
  [GLFW_KEY_Z] = KeyCode_Z,
  [GLFW_KEY_LEFT_BRACKET] = KeyCode_LeftBracket,
  [GLFW_KEY_BACKSLASH] = KeyCode_Backslash,
  [GLFW_KEY_RIGHT_BRACKET] = KeyCode_RightBrace,
  [GLFW_KEY_GRAVE_ACCENT] = KeyCode_Invalid,
  [GLFW_KEY_WORLD_1] = KeyCode_Invalid,
  [GLFW_KEY_WORLD_2] = KeyCode_Invalid,
  [GLFW_KEY_ESCAPE] = KeyCode_Esc,
  [GLFW_KEY_ENTER] = KeyCode_Enter,
  [GLFW_KEY_TAB] = KeyCode_Tab,
  [GLFW_KEY_BACKSPACE] = KeyCode_Backspace,
  [GLFW_KEY_INSERT] = KeyCode_Invalid,
  [GLFW_KEY_DELETE] = KeyCode_Delete,
  [GLFW_KEY_RIGHT] = KeyCode_RightArrow,
  [GLFW_KEY_LEFT] = KeyCode_LeftArrow,
  [GLFW_KEY_DOWN] = KeyCode_DownArrow,
  [GLFW_KEY_UP] = KeyCode_UpArrow,
  [GLFW_KEY_PAGE_UP] = KeyCode_PageUp,
  [GLFW_KEY_PAGE_DOWN] = KeyCode_PageDown,
  [GLFW_KEY_HOME] = KeyCode_Invalid,
  [GLFW_KEY_END] = KeyCode_Invalid,
  [GLFW_KEY_CAPS_LOCK] = KeyCode_CapsLock,
  [GLFW_KEY_SCROLL_LOCK] = KeyCode_Invalid,
  [GLFW_KEY_NUM_LOCK] = KeyCode_Invalid,
  [GLFW_KEY_PRINT_SCREEN] = KeyCode_Invalid,
  [GLFW_KEY_PAUSE] = KeyCode_Invalid,
  [GLFW_KEY_F1] = KeyCode_F1,
  [GLFW_KEY_F2] = KeyCode_F2,
  [GLFW_KEY_F3] = KeyCode_F3,
  [GLFW_KEY_F4] = KeyCode_F4,
  [GLFW_KEY_F5] = KeyCode_F5,
  [GLFW_KEY_F6] = KeyCode_F6,
  [GLFW_KEY_F7] = KeyCode_F7,
  [GLFW_KEY_F8] = KeyCode_F8,
  [GLFW_KEY_F9] = KeyCode_F9,
  [GLFW_KEY_F10] = KeyCode_Invalid,
  [GLFW_KEY_F11] = KeyCode_Invalid,
  [GLFW_KEY_F12] = KeyCode_Invalid,
  [GLFW_KEY_F13] = KeyCode_Invalid,
  [GLFW_KEY_F14] = KeyCode_Invalid,
  [GLFW_KEY_F15] = KeyCode_Invalid,
  [GLFW_KEY_F16] = KeyCode_Invalid,
  [GLFW_KEY_F17] = KeyCode_Invalid,
  [GLFW_KEY_F18] = KeyCode_Invalid,
  [GLFW_KEY_F19] = KeyCode_Invalid,
  [GLFW_KEY_F20] = KeyCode_Invalid,
  [GLFW_KEY_F21] = KeyCode_Invalid,
  [GLFW_KEY_F22] = KeyCode_Invalid,
  [GLFW_KEY_F23] = KeyCode_Invalid,
  [GLFW_KEY_F24] = KeyCode_Invalid,
  [GLFW_KEY_F25] = KeyCode_Invalid,
  [GLFW_KEY_KP_0] = KeyCode_Invalid,
  [GLFW_KEY_KP_1] = KeyCode_Invalid,
  [GLFW_KEY_KP_2] = KeyCode_Invalid,
  [GLFW_KEY_KP_3] = KeyCode_Invalid,
  [GLFW_KEY_KP_4] = KeyCode_Invalid,
  [GLFW_KEY_KP_5] = KeyCode_Invalid,
  [GLFW_KEY_KP_6] = KeyCode_Invalid,
  [GLFW_KEY_KP_7] = KeyCode_Invalid,
  [GLFW_KEY_KP_8] = KeyCode_Invalid,
  [GLFW_KEY_KP_9] = KeyCode_Invalid,
  [GLFW_KEY_KP_DECIMAL] = KeyCode_Invalid,
  [GLFW_KEY_KP_DIVIDE] = KeyCode_Invalid,
  [GLFW_KEY_KP_MULTIPLY] = KeyCode_Invalid,
  [GLFW_KEY_KP_SUBTRACT] = KeyCode_Invalid,
  [GLFW_KEY_KP_ADD] = KeyCode_Invalid,
  [GLFW_KEY_KP_ENTER] = KeyCode_Invalid,
  [GLFW_KEY_KP_EQUAL] = KeyCode_Invalid,
  [GLFW_KEY_LEFT_SHIFT] = KeyCode_LeftShift,
  [GLFW_KEY_LEFT_CONTROL] = KeyCode_LeftCtrl,
  [GLFW_KEY_LEFT_ALT] = KeyCode_Alt,
  [GLFW_KEY_LEFT_SUPER] = KeyCode_Invalid,
  [GLFW_KEY_RIGHT_SHIFT] = KeyCode_RightShift,
  [GLFW_KEY_RIGHT_CONTROL] = KeyCode_RightCtrl,
  [GLFW_KEY_RIGHT_ALT] = KeyCode_Alt,
  [GLFW_KEY_RIGHT_SUPER] = KeyCode_Invalid,
  [GLFW_KEY_MENU] = KeyCode_Invalid,
  
};

Key_Code
osx_translate_key(int glfw_key)
{
  // TODO: turn this into an actual key_code
  return glfw_key_translation_table[glfw_key];
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
  if (!glfwInit())
  {
    printf("Error: Could not initialize glfw.\n");
    return 1;
  }
  glfwSetErrorCallback(glfw_error_callback);
  
  s32 init_window_width = 1400 / 2;
  s32 init_window_height = 700 / 2;
  
  glfwWindowHint(GLFW_DOUBLEBUFFER, true);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  GLFWwindow* window = glfwCreateWindow(init_window_width, init_window_height, "Lumenarium", NULL, NULL);
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
  
  Editor_Desc ed_desc = {};
  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  ed_desc.content_scale = (v2){ xscale, yscale };
  ed_desc.init_window_dim = (v2){init_window_width, init_window_height};
  
  App_State* state = lumenarium_init(&ed_desc);
  app_state_data = (u8*)state;
  
  bool running = true;
  r64 target_seconds_per_frame = state->target_seconds_per_frame;
  Ticks ticks_start = os_get_ticks();
  while(!glfwWindowShouldClose(window) && running && has_flag(state->flags, AppState_IsRunning)) {
    lumenarium_frame_prepare(state);
    glfwPollEvents();
    
    if (has_flag(state->flags, AppState_RunEditor))
    {
      s32 w, h;
      glfwGetWindowSize(window, &w, &h);
      state->editor->window_dim = (v2){ (r32)w, (r32)h };
    }
    
    lumenarium_frame(state);
    lumenarium_env_validate();
    
    glfwSwapBuffers(window);
    
    Ticks ticks_end = os_get_ticks();
    r64 seconds_elapsed = get_seconds_elapsed(ticks_start, ticks_end, os_get_ticks_per_second());
    while (seconds_elapsed < target_seconds_per_frame)
    {
      u32 sleep_time = (u32)(1000000.0f * (target_seconds_per_frame - seconds_elapsed));
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