typedef u32 Win32_Window_Event_Flags;
enum 
{
  WindowEventFlag_None = 0,
  WindowEventFlag_CloseRequested = 1,
  WindowEventFlag_WindowIsActive = 2,
};

struct Win32_Window_OpenGL_Info
{
  BYTE bits_color;
  BYTE bits_alpha;
  BYTE bits_depth;
  
  HGLRC rc;
};

struct Win32_Window_Info
{
  char* name;
  char* class_name;
  s32 width;
  s32 height;
};

struct Win32_Window
{
  Win32_Window_Info info;
  WNDCLASSEX window_class;
  WNDPROC window_event_handler;
  HWND window_handle;
  HDC dc;
  Win32_Window_OpenGL_Info opengl_info;
};

//////////////////////////////////////////
//   Main Window
//
// At the moment, we only need one window, so this is easier to 
// track globally. Replace this if we need more windows

global Win32_Window_Event_Flags win32_window_event_flags = 0;
global Win32_Window win32_main_window = {};

//////////////////////////////////////////
//

internal bool
win32_window_create(
                    Win32_Window* dest,
                    HINSTANCE hinstance, 
                    char* window_name, 
                    s32 width, 
                    s32 height,
                    WNDPROC window_event_handler
                    )
{
  dest->info.name = window_name;
  dest->info.class_name = window_name;
  dest->info.width = width;
  dest->info.height = height;
  
  dest->window_event_handler = window_event_handler;
  
  dest->window_class = {};
  dest->window_class.cbSize = sizeof(WNDCLASSEX);
  dest->window_class.style = (
                              CS_HREDRAW | 
                              CS_VREDRAW | 
                              CS_OWNDC // TODO(PS): need to know what this is
                              );
  dest->window_class.lpfnWndProc = window_event_handler;
  dest->window_class.cbClsExtra = 0;
  dest->window_class.cbWndExtra = 0;
  dest->window_class.hInstance = hinstance;
  dest->window_class.hIcon = NULL;
  dest->window_class.hCursor = NULL;
  dest->window_class.hbrBackground = NULL;
  dest->window_class.lpszMenuName = 0;
  dest->window_class.lpszClassName = window_name; // "main_window_class";
  dest->window_class.hIconSm = NULL;
  
  if (RegisterClassEx(&dest->window_class))
  {
    dest->window_handle = CreateWindowEx(
                                         0,
                                         dest->window_class.lpszClassName,
                                         window_name,
                                         WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         width,
                                         height,
                                         0,
                                         0,
                                         hinstance,
                                         0
                                         );
    return true;
  }
  return false;
}

internal void
win32_window_update_dim(Win32_Window* win)
{
  RECT client_rect;
  GetClientRect(win->window_handle, &client_rect);
  win->info.width = client_rect.right - client_rect.left;
  win->info.height = client_rect.bottom - client_rect.top;
}

internal void win32_window_opengl_ctx_create(Win32_Window* win, Win32_Window_OpenGL_Info info, HINSTANCE hinstance);

LRESULT CALLBACK
win32_window_event_handler(HWND window_handle, UINT msg, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  
  switch (msg)
  {
    case WM_CREATE:
    {
      win32_main_window.dc = GetDC(window_handle);
      HINSTANCE hinstance = GetModuleHandle(NULL);
      win32_window_opengl_ctx_create(&win32_main_window, { 32, 8, 0 }, hinstance);
    }break;
    
    case WM_SIZE:
    {
      win32_window_update_dim(&win32_main_window);
    }break;
    
    case WM_CLOSE:
    {
      if (win32_main_window.opengl_info.rc)
      {
        wglDeleteContext(win32_main_window.opengl_info.rc);
        win32_main_window.opengl_info.rc = NULL;
      }
      if (win32_main_window.dc)
      {
        ReleaseDC(win32_main_window.window_handle, win32_main_window.dc);
        win32_main_window.dc = NULL;
      }
      
      add_flag(win32_window_event_flags, WindowEventFlag_CloseRequested);
      running = false;
      
      DestroyWindow(win32_main_window.window_handle);
    }break;
    
    case WM_DESTROY:
    {
      if (win32_main_window.opengl_info.rc)
      {
        wglDeleteContext(win32_main_window.opengl_info.rc);
        win32_main_window.opengl_info.rc = NULL;
      }
      if (win32_main_window.dc)
      {
        ReleaseDC(win32_main_window.window_handle, win32_main_window.dc);
        win32_main_window.dc = NULL;
      }
      
      //PostQuitMessage(0);
      //result = DefWindowProc(window_handle, msg, wparam, lparam);
    }break;
    
    case WM_PAINT:
    {
      PAINTSTRUCT paint_struct;
      HDC device_ctx;
      b32 paint_result;
      
      device_ctx = BeginPaint(window_handle, &paint_struct);
      paint_result   = EndPaint(window_handle, &paint_struct);
    }break;
    
    case WM_ACTIVATE:
    {
      bool WindowIsActive = (
                             LOWORD(wparam) == WA_ACTIVE || LOWORD(wparam) == WA_CLICKACTIVE
                             );
      if (WindowIsActive)
      {
        add_flag(win32_window_event_flags, WindowEventFlag_WindowIsActive);
      }
      else
      {
        rem_flag(win32_window_event_flags, WindowEventFlag_WindowIsActive);
      }
    }break;
    
    default:
    {
      result = DefWindowProc(window_handle, msg, wparam, lparam);
    }
  }
  
  return result;
}

////////////////////////////////////////////
// OpenGL dummy window functions

LRESULT CALLBACK
win32_opengl_event_handler(HWND window_handle, UINT msg, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  
  switch (msg)
  {
    case WM_CREATE:  { }break;
    case WM_CLOSE:   { DestroyWindow(window_handle); }break;
    case WM_DESTROY: { } break;
    default: { return DefWindowProc(window_handle, msg, wparam, lparam); } break;
  }
  
  return result;
}

internal void
win32_window_opengl_ctx_create_no_ext(HDC dc, Win32_Window_OpenGL_Info* info)
{
  // Setup pixel format
  {
    PIXELFORMATDESCRIPTOR pixel_format_desc = { 0 };
    pixel_format_desc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixel_format_desc.nVersion = 1;
    pixel_format_desc.dwFlags = (
                                 PFD_SUPPORT_OPENGL | 
                                 PFD_DRAW_TO_WINDOW | 
                                 PFD_DOUBLEBUFFER
                                 );
    
    pixel_format_desc.cColorBits = info->bits_color;
    pixel_format_desc.cAlphaBits = info->bits_alpha;
    pixel_format_desc.cDepthBits = info->bits_depth;
    pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
    pixel_format_desc.dwLayerMask = PFD_MAIN_PLANE;
    
    s32 pixel_fmt = ChoosePixelFormat(dc, &pixel_format_desc);
    if (pixel_fmt == 0) 
    {
      win32_get_last_error();
      invalid_code_path;
    }
    if ((pixel_fmt = ChoosePixelFormat(dc, &pixel_format_desc)) == 0)
    {
      win32_get_last_error();
      invalid_code_path;
    }
    if (SetPixelFormat(dc, pixel_fmt, &pixel_format_desc) == FALSE) 
    { 
      win32_get_last_error();
      invalid_code_path; 
    }
  }
  
  // Create rendering context
  {
    info->rc = wglCreateContext(dc);
    if (info->rc == NULL) {
      win32_get_last_error();
      invalid_code_path;
    }
    if (!wglMakeCurrent(dc, info->rc))
    {
      win32_get_last_error();
      invalid_code_path;
    }
  }
}

// Based on documentation at:
//       https://www.opengl.org/wiki/Creating_an_OpenGL_Context_(WGL)#Proper_Context_Creation
internal void
win32_window_opengl_ctx_create_ext(HDC dc, Win32_Window_OpenGL_Info* info)
{
  const int pixel_fmt_attribs[] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
    WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
    WGL_COLOR_BITS_ARB,     32,
    WGL_DEPTH_BITS_ARB,     24,
    WGL_STENCIL_BITS_ARB,   8,
    0, 0
  };
  
  s32 pixel_fmt;
  u32 num_formats;
  
  // this will contain the description of pixel_fmt after we set it
  PIXELFORMATDESCRIPTOR pixel_format_desc = { 0 };
  
  if (!gl.wglChoosePixelFormatARB(dc, pixel_fmt_attribs, NULL, 1, &pixel_fmt, &num_formats))
  {
    win32_get_last_error();
    invalid_code_path;
  }
  if (SetPixelFormat(dc, pixel_fmt, &pixel_format_desc) == FALSE) 
  { 
    win32_get_last_error();
    invalid_code_path; 
  }
  
  int ctx_flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
  
#if defined(DEBUG)
  ctx_flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif
  
  const int ctx_attribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_FLAGS_ARB, ctx_flags,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    
    0, 0,
  };
  
  info->rc = gl.wglCreateContextAttribsARB(dc, 0, ctx_attribs);
  if (info->rc == NULL) {
    win32_get_last_error();
    invalid_code_path;
  }
  if (!wglMakeCurrent(dc, info->rc))
  {
    win32_get_last_error();
    invalid_code_path;
  }
  
  char* version_string = (char*)glGetString(GL_VERSION);
  err_write("OpenGL Version: %s\n", version_string);
}

internal void
win32_window_opengl_make_current(Win32_Window* win)
{
  
}

#define wgl_load_ext(e,n) e.n = (proc_##n*)wglGetProcAddress(#n); assert((e.n) != 0)

internal Win32_OpenGL_Extensions
win32_window_opengl_get_wgl_ext(HINSTANCE hinstance)
{
  Win32_OpenGL_Extensions result = {0};
  
  WNDCLASSEX window_class = {};
  window_class.cbSize = sizeof(WNDCLASSEX);
  window_class.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC);
  window_class.lpfnWndProc = win32_opengl_event_handler;
  window_class.hInstance = hinstance;
  window_class.lpszClassName = "opengl_window_class";
  if (RegisterClassEx(&window_class))
  {
    HWND window_handle = CreateWindowEx(0, window_class.lpszClassName,
                                        "opengl_window", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        CW_USEDEFAULT, CW_USEDEFAULT, 32, 32, 0, 0, hinstance, 0
                                        );
    
    HDC dc = GetDC(window_handle);
    Win32_Window_OpenGL_Info info = { 32, 24, 8 };
    win32_window_opengl_ctx_create_no_ext(dc, &info);
    
    wgl_load_ext(result, wglGetExtensionsStringARB);
    if (result.wglGetExtensionsStringARB != 0)
    {
      const char* extension_string = result.wglGetExtensionsStringARB(dc);
      OutputDebugStringA("OpenGL Extensions: \n");
      OutputDebugStringA(extension_string);
      OutputDebugStringA("\n\n");
      
      wgl_load_ext(result, wglChoosePixelFormatARB);
      wgl_load_ext(result, wglCreateContextAttribsARB);
      wgl_load_ext(result, glGenVertexArrays);
      wgl_load_ext(result, glBindVertexArray);
      wgl_load_ext(result, glGenBuffers);
      wgl_load_ext(result, glBindBuffer);
      wgl_load_ext(result, glBufferData);
      wgl_load_ext(result, glCreateShader);
      wgl_load_ext(result, glShaderSource);
      wgl_load_ext(result, glCompileShader);
      wgl_load_ext(result, glCreateProgram);
      wgl_load_ext(result, glAttachShader);
      wgl_load_ext(result, glLinkProgram);
      wgl_load_ext(result, glUseProgram);
      wgl_load_ext(result, glGetAttribLocation);
      wgl_load_ext(result, glVertexAttribPointer);
      wgl_load_ext(result, glEnableVertexAttribArray);
      wgl_load_ext(result, glGetShaderiv);
      wgl_load_ext(result, glGetShaderInfoLog);
      wgl_load_ext(result, glGetProgramiv);
      wgl_load_ext(result, glGetProgramInfoLog);
    }
    
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(info.rc);
    ReleaseDC(window_handle, dc);
    DestroyWindow(window_handle);
  }
  
  return result;
}

internal void
win32_window_opengl_ctx_create(Win32_Window* win, Win32_Window_OpenGL_Info info, HINSTANCE hinstance)
{
  if (gl.wglGetExtensionsStringARB == 0)
  {
    gl = win32_window_opengl_get_wgl_ext(hinstance);
  }
  
  win32_window_opengl_ctx_create_ext(win->dc, &info);
  win->opengl_info = info;
}
