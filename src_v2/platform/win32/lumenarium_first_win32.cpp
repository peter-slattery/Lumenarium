
#include "../lumenarium_compiler_flags.h"
#include "../lumenarium_platform_common_includes.h"

#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>

#include "../../lumenarium_types.h"
#include "../lumenarium_platform.h"
#include "../../lumenarium_first.cpp"

global DWORD win32_last_error = 0;
void
win32_get_last_error()
{
  win32_last_error = GetLastError();
}

global bool running = true;

#include "lumenarium_win32_opengl.h"

global Win32_OpenGL_Extensions gl;

#include "lumenarium_win32_memory.cpp"
#include "lumenarium_win32_window.cpp"
#include "lumenarium_win32_time.cpp"
#include "lumenarium_win32_file.cpp"
#include "lumenarium_win32_thread.cpp"
#include "lumenarium_win32_graphics.cpp"

internal Platform_Key_Flags
win32_get_key_flags_mod()
{
  Platform_Key_Flags result = 0;
  if (GetKeyState(VK_SHIFT)   & 0x8000) add_flag(result, KeyFlag_Mod_Shift);
  if (GetKeyState(VK_MENU)    & 0x8000) add_flag(result, KeyFlag_Mod_Alt);
  if (GetKeyState(VK_CONTROL) & 0x8000) add_flag(result, KeyFlag_Mod_Ctrl);
  return result;
}

internal void 
win32_mouse_capture(Win32_Window* win) 
{ 
  // NOTE(Peter): We capture events when the mouse goes down so that
  // if the user drags outside the window, we still get the mouse up
  // event and can process it. Otherwise, we can get into cases where
  // an event was started, didn't end, but the user can click again and
  // try to start the event again.
  // We relase event capture on mouse up.
  SetCapture(win->window_handle); 
}

internal void 
win32_mouse_release(Win32_Window* win) 
{ 
  ReleaseCapture();
}

internal Platform_Window_Event
win32_button_event(Platform_Key_Code key, bool is_down, bool was_down)
{
  Platform_Window_Event evt = {};
  evt.kind = WindowEvent_ButtonDown;
  evt.key_code = key;
  evt.key_flags = win32_get_key_flags_mod();
  if (is_down)  add_flag(evt.key_flags, KeyFlag_State_IsDown);
  if (was_down) add_flag(evt.key_flags, KeyFlag_State_WasDown);
  return evt;
}

internal void
win32_window_handle_event(MSG msg, Win32_Window* win, App_State* state)
{
  switch (msg.message)
  {
    case WM_MOUSEWHEEL:
    {
      Platform_Window_Event evt = {};
      evt.kind = WindowEvent_MouseScroll;
      evt.scroll_amt = GET_WHEEL_DELTA_WPARAM(msg.wParam);
      lumenarium_event(evt, state);
    }break;
    
    case WM_LBUTTONDOWN:
    {
      Platform_Window_Event evt = win32_button_event(
                                                     KeyCode_MouseLeftButton, 
                                                     true, false
                                                     );
      lumenarium_event(evt, state);
      win32_mouse_capture(win);
    }break;
    
    case WM_MBUTTONDOWN:
    {
      Platform_Window_Event evt = win32_button_event(
                                                     KeyCode_MouseMiddleButton, 
                                                     true, false
                                                     );
      lumenarium_event(evt, state);
      win32_mouse_capture(win);
    }break;
    
    case WM_RBUTTONDOWN:
    {
      Platform_Window_Event evt = win32_button_event(
                                                     KeyCode_MouseRightButton, 
                                                     true, false
                                                     );
      lumenarium_event(evt, state);
      win32_mouse_capture(win);
    }break;
    
    case WM_LBUTTONUP:
    {
      Platform_Window_Event evt = win32_button_event(
                                                     KeyCode_MouseLeftButton, 
                                                     false, true
                                                     );
      lumenarium_event(evt, state);
      win32_mouse_release(win);
    }break;
    
    case WM_MBUTTONUP:
    {
      Platform_Window_Event evt = win32_button_event(
                                                     KeyCode_MouseMiddleButton, 
                                                     false, true
                                                     );
      lumenarium_event(evt, state);
      win32_mouse_release(win);
    }break;
    
    case WM_RBUTTONUP:
    {
      Platform_Window_Event evt = win32_button_event(
                                                     KeyCode_MouseRightButton, 
                                                     false, true
                                                     );
      lumenarium_event(evt, state);
      win32_mouse_release(win);
    }break;
    
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      Platform_Key_Code key = 0;
      b32 was_down = (msg.lParam & (1 << 30)) != 0;
      b32 is_down  = (msg.lParam & (1 << 31)) == 0;
      Platform_Window_Event evt = win32_button_event(key, is_down, was_down);
      lumenarium_event(evt, state);
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }break;
    
    case WM_CHAR:
    {
      Platform_Window_Event evt = {};
      evt.kind = WindowEvent_Char;
      evt.char_value = (char)msg.wParam;
      lumenarium_event(evt, state);      
    }break;
    
    default:
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }break;
  }
}

INT WINAPI 
WinMain(
        HINSTANCE hInstance, 
        HINSTANCE hPrevInstance,
        PSTR lpCmdLine, 
        INT nCmdShow)
{
  // Window Setup
  win32_window_create(
                      &win32_main_window, 
                      hInstance, 
                      "Lumenariumtest0", 
                      1600, 
                      900, 
                      win32_window_event_handler
                      );
  
  win32_time_init();
  win32_files_init();
  win32_threads_init();
  
  App_State* state = lumenarium_init();
  if (!has_flag(state->flags, AppState_IsRunning)) return 0;
  
  Platform_Ticks ticks_start = platform_get_ticks();
  while (running && has_flag(state->flags, AppState_IsRunning))
  {
    win32_threads_reclaim();
    lumenarium_frame_prepare(state);
    
    // Potentially pass the window closed event to the runtime
    if (win32_window_event_flags & WindowEventFlag_CloseRequested)
    {
      Platform_Window_Event evt = {
        WindowEvent_WindowClosed,
      };
      lumenarium_event(evt, state);
    }
    
    // Pass Window Events to the runtime
    MSG window_msg;
    while (PeekMessageA(
                        &window_msg, 
                        win32_main_window.window_handle, 
                        0, 
                        0, 
                        PM_REMOVE)
           ){
      win32_window_handle_event(window_msg, &win32_main_window, state);
    }
    
    // NOTE(PS): WM_CLOSE and WM_DESTROY can both be issued 
    // the same frame, meaning our drawing context is destroyed
    // before calling lumenarium_frame so skipping here to avoid
    // using invalid resources
    if (!running || !has_flag(state->flags, AppState_IsRunning)) continue;
    
    lumenarium_frame(state);
    
    SwapBuffers(win32_main_window.dc);
    
    ////////////////////////////////////////
    //  Maintain Frame Rate
    
    Platform_Ticks ticks_end = platform_get_ticks();
    r64 seconds_elapsed = get_seconds_elapsed(ticks_start, ticks_end);
    while (seconds_elapsed < target_seconds_per_frame)
    {
      u32 sleep_time = (u32)(1000.0f * (target_seconds_per_frame - seconds_elapsed));
      Sleep(sleep_time);
      
      ticks_end = platform_get_ticks();
      seconds_elapsed = get_seconds_elapsed(ticks_start, ticks_end);
    }
    ticks_start = ticks_end;
  }
  
  lumenarium_cleanup(state);
  
  // threads cleanup
  for (u32 i = 1; i < win32_threads_cap; i++)
  {
    if (win32_threads[i] == INVALID_HANDLE_VALUE) continue;
    TerminateThread(win32_threads[i], 0);
  }
  
  // windows cleanup
  UnregisterClass(win32_main_window.window_class.lpszClassName, hInstance);
  return 0;
}

