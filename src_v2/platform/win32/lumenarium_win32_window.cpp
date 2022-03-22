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

struct Win32_Window
{
  char* name;
  char* class_name;
  s32 width;
  s32 height;
  
  WNDCLASS window_class;
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

internal Win32_Window
win32_window_create(
                    HINSTANCE hinstance, 
                    char* window_name, 
                    s32 width, 
                    s32 height,
                    WNDPROC window_event_handler
                    )
{
  Win32_Window result = {};
  result.name = window_name;
  result.class_name = window_name;
  result.width = width;
  result.height = height;
  result.window_event_handler = window_event_handler;
  
  result.window_class = {};
  result.window_class.style = CS_HREDRAW | CS_VREDRAW;
  result.window_class.lpfnWndProc = window_event_handler;
  result.window_class.hInstance = hinstance;
  result.window_class.lpszClassName = window_name;
  
  if (RegisterClass(&result.window_class))
  {
    result.window_handle = CreateWindowEx(
                                          0,
                                          result.window_class.lpszClassName,
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
    result.dc = GetDC(result.window_handle);
  }
  
  return result;
}

internal void
win32_window_update_dim(Win32_Window* win)
{
  RECT client_rect;
  GetClientRect(win->window_handle, &client_rect);
  win->width = client_rect.right - client_rect.left;
  win->height = client_rect.bottom - client_rect.top;
}

LRESULT CALLBACK
win32_window_event_handler(HWND window_handle, UINT msg, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  
  switch (msg)
  {
    case WM_SIZE:
    {
      win32_window_update_dim(&win32_main_window);
    }break;
    
    case WM_CLOSE:
    {
      result = DefWindowProc(window_handle, msg, wparam, lparam);
      add_flag(win32_window_event_flags, WindowEventFlag_CloseRequested);
    }break;
    
    case WM_DESTROY:
    {
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

internal void
win32_window_opengl_ctx_create(Win32_Window* win, Win32_Window_OpenGL_Info info)
{
  // Setup pixel format
  {
    PIXELFORMATDESCRIPTOR pixel_format_desc = { 0 };
    // TODO: Program seems to work perfectly fine without all other params except dwFlags.
    //       Can we skip other params for the sake of brevity?
    pixel_format_desc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixel_format_desc.nVersion = 1;
    pixel_format_desc.dwFlags = (
                                 PFD_SUPPORT_OPENGL | 
                                 PFD_DRAW_TO_WINDOW | 
                                 PFD_DOUBLEBUFFER
                                 );
    pixel_format_desc.cColorBits = info.bits_color;
    pixel_format_desc.cAlphaBits = info.bits_alpha;
    pixel_format_desc.cDepthBits = info.bits_depth;
    
    // TODO(Peter): include these in win32_opengl_window_info?
    pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
    pixel_format_desc.dwLayerMask = PFD_MAIN_PLANE;
    
    s32 pixel_fmt = ChoosePixelFormat(win->dc, &pixel_format_desc);
    if (!pixel_fmt) { invalid_code_path; }
    if (!SetPixelFormat(win->dc, pixel_fmt, &pixel_format_desc)) 
    { 
      invalid_code_path; 
    }
  }
  
  // Create rendering context
  {
    // TODO: Create "proper" context?
    //       https://www.opengl.org/wiki/Creating_an_OpenGL_Context_(WGL)#Proper_Context_Creation
    
    info.rc = wglCreateContext(win->dc);
    wglMakeCurrent(win->dc, info.rc);
    
    // TODO(Peter): do we want this?
    /*
        glGetIntegerv(GL_MAJOR_VERSION, );
        glGetIntegerv(GL_MINOR_VERSION, );
        (char*)glGetString(GL_VENDOR);
        (char*)glGetString(GL_RENDERER);
        */
  }
  
  win->opengl_info = info;
}
