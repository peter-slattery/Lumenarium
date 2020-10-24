//
// File: gs_win32.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef GS_WIN32_CPP

#ifndef GS_WIN32_CPP

struct win32_state
{
    b32 Initialized;
    b32 Running;
};

struct win32_opengl_window_info
{
    s32 ColorBits;
    s32 AlphaBits;
    s32 DepthBits;
    HGLRC RenderContext;
};

struct window
{
    char* Name;
    char* ClassName;
    s32 Width;
    s32 Height;
    WNDPROC WindowEventHandler;
    
    WNDCLASS Class;
    HWND Handle;
    HDC DeviceContext;
    
    // TODO(peter): Make this a union?
    win32_opengl_window_info OpenGLInfo;
};

struct handle_window_msg_result
{
    b32 NeedsUpdate;
#ifdef DEBUG
    char MessageType[128];
#endif
};

global win32_state GlobalWin32State;

// Utility
internal s32 Win32StringLength(char* String);
internal s32 Win32ConcatStrings(s32 ALen, char* A, s32 BLen, char* B, s32 DestLen, char* Dest);

// Windowing & Graphics
struct win32_offscreen_buffer
{
    texture_buffer Buffer;
    BITMAPINFO Info;
};

LRESULT CALLBACK      Win32HandleWindowsEvents (HWND WindowHandle, UINT Msg, WPARAM wParam, LPARAM lParam);
internal handle_window_msg_result HandleWindowsMessage (HWND WindowHandle, MSG Message);
internal void          Win32UpdateWindowDimension(window* Window);
internal void          Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height);
internal void          Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, window Window);

// Memory

internal ALLOCATOR_ALLOC(Win32Alloc);
internal ALLOCATOR_FREE(Win32Free);

///
// Utils
///

internal s32
Win32StringLength(char* String)
{
    char* At = String;
    while (*At) { At++; };
    return At - String;
}

internal s32
Win32ConcatStrings(s32 ALength, char* A, s32 BLength, char* B, s32 DestLength, char* Dest)
{
    char* Dst = Dest;
    char* AAt = A;
    int ALengthToCopy = ALength < DestLength ? ALength : DestLength;
    for (s32 a = 0; a < ALength; a++)
    {
        *Dst++ = *AAt++;
    }
    char* BAt = B;
    int DestLengthRemaining = DestLength - (Dst - Dest);
    int BLengthToCopy = BLength < DestLengthRemaining ? BLength : DestLength;
    for (s32 b = 0; b < BLengthToCopy; b++)
    {
        *Dst++ = *BAt++;
    }
    int DestLengthOut = Dst - Dest;
    int NullTermIndex = DestLengthOut < DestLength ? DestLengthOut : DestLength;
    Dest[NullTermIndex] = 0;
    return DestLengthOut;
}

///
// Windowing
///

internal window
Win32CreateWindow (HINSTANCE HInstance, char* WindowName, s32 Width, s32 Height,
                   WNDPROC WindowEventHandler)
{
    window Result = {};
    Result.Name = WindowName;
    Result.ClassName = WindowName;
    Result.Width = Width;
    Result.Height = Height;
    Result.WindowEventHandler = WindowEventHandler;
    
    Result.Class = {};
    Result.Class.style = CS_HREDRAW | CS_VREDRAW;
    Result.Class.lpfnWndProc = WindowEventHandler;
    Result.Class.hInstance = HInstance;
    Result.Class.lpszClassName = WindowName;
    
    if (RegisterClass(&Result.Class))
    {
        Result.Handle = CreateWindowEx(
                                       0,
                                       Result.Class.lpszClassName,
                                       WindowName,
                                       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       Width,
                                       Height,
                                       0,
                                       0,
                                       HInstance,
                                       0);
        Result.DeviceContext = GetDC(Result.Handle);
    }
    
    return Result;
};

internal window
PlatformCreateWindow (char* WindowName, s32 Width, s32 Height)
{
    HINSTANCE HInstance = GetModuleHandle(NULL);
    return Win32CreateWindow(HInstance, WindowName, Width, Height, Win32HandleWindowsEvents);
}

internal void
CreateOpenGLWindowContext (win32_opengl_window_info Info, window* Window)
{
    // Setup pixel format
    {
        PIXELFORMATDESCRIPTOR PixelFormatDesc = { 0 };
        // TODO: Program seems to work perfectly fine without all other params except dwFlags.
        //       Can we skip other params for the sake of brevity?
        PixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        PixelFormatDesc.nVersion = 1;
        PixelFormatDesc.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        PixelFormatDesc.iPixelType = PFD_TYPE_RGBA;// TODO(Peter): include this in win32_opengl_window_info?
        PixelFormatDesc.cColorBits = Info.ColorBits;
        PixelFormatDesc.cAlphaBits = Info.AlphaBits;
        PixelFormatDesc.cDepthBits = Info.DepthBits;
        PixelFormatDesc.dwLayerMask = PFD_MAIN_PLANE; // TODO(Peter): include this in win32_opengl_window_info?
        //
        
        s32 PixelFormat = ChoosePixelFormat(Window->DeviceContext, &PixelFormatDesc);
        if (!PixelFormat) { InvalidCodePath; } // TODO: Log: Choose pixel format failed
        if (!SetPixelFormat(Window->DeviceContext, PixelFormat, &PixelFormatDesc)) { InvalidCodePath; } // TODO: Log: Set pixel format failed
    }
    
    // Create rendering context
    {
        // TODO: Create "proper" context?
        //       https://www.opengl.org/wiki/Creating_an_OpenGL_Context_(WGL)#Proper_Context_Creation
        
        Info.RenderContext = wglCreateContext(Window->DeviceContext);
        wglMakeCurrent(Window->DeviceContext, Info.RenderContext);
        
        // TODO(Peter): do we want this?
        /*
            glGetIntegerv(GL_MAJOR_VERSION, );
            glGetIntegerv(GL_MINOR_VERSION, );
            (char*)glGetString(GL_VENDOR);
            (char*)glGetString(GL_RENDERER);
            */
    }
    
    Window->OpenGLInfo = Info;
}

struct handle_window_event_result
{
    LRESULT Result;
    b32 Handled;
};

internal void
Win32UpdateInputFrameMouseButtonState (input_frame* InputFrame, key_code KeyCode, int Win32VirtualKey)
{
    InputFrame->KeysDown[KeyCode] = (GetKeyState(Win32VirtualKey) & (1 << 15)) != 0;
}

internal void
Win32UpdateInputFrameMouseState (input_frame* InputFrame)
{
    Win32UpdateInputFrameMouseButtonState(InputFrame, KeyCode_MouseLeftButton, VK_LBUTTON);
    Win32UpdateInputFrameMouseButtonState(InputFrame, KeyCode_MouseMiddleButton, VK_MBUTTON);
    Win32UpdateInputFrameMouseButtonState(InputFrame, KeyCode_MouseRightButton, VK_RBUTTON);
    // NOTE(Peter): If you decide to support extra mouse buttons, on windows the key codes are
    // VK_XBUTTON1 and VK_XBUTTON2
}

internal void
Win32UpdateInputFrameMouseWheelDelta (input_frame* InputFrame, MSG Message)
{
    int MouseWheel = GET_WHEEL_DELTA_WPARAM(Message.wParam);
    InputFrame->MouseScroll = MouseWheel;
}

internal handle_window_event_result
HandleWindowEventUnlessWouldUseDefault (HWND WindowHandle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    handle_window_event_result Result = {};
    Result.Handled = false;
    
    switch (Msg)
    {
        case WM_SIZE:
        {
            
            Result.Handled = true;
        }break;
        
        case WM_CLOSE:
        {
            Result.Result = DefWindowProc(WindowHandle, Msg, wParam, lParam);
            Result.Handled = true;
        }break;
        
        case WM_DESTROY:
        {
            GlobalWin32State.Running = false;
            Result.Handled = true;
        }break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            HDC DeviceContext;
            b32 PaintResult;
            
            DeviceContext = BeginPaint(WindowHandle, &PaintStruct);
            PaintResult = EndPaint(WindowHandle, &PaintStruct);
            Result.Handled = true;
        }break;
    }
    
    return Result;
}

LRESULT CALLBACK
Win32HandleWindowsEvents (
                          HWND WindowHandle,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam
                          )
{
    handle_window_event_result EventResult = HandleWindowEventUnlessWouldUseDefault(
                                                                                    WindowHandle,
                                                                                    Msg,
                                                                                    wParam,
                                                                                    lParam);
    
    if (!EventResult.Handled)
    {
        EventResult.Result = DefWindowProc(WindowHandle, Msg, wParam, lParam);
    }
    
    return EventResult.Result;
}

static key_code
Win32GetKeyCode (int Win32VirtualKey, bool NumpadValid, bool TranslateToChar)
{
    key_code Result = KeyCode_Invalid;
    
    if (Win32VirtualKey == VK_ESCAPE) { Result = KeyCode_Esc; }
    
    if (!TranslateToChar)
    {
        if      (Win32VirtualKey == VK_SPACE)       { Result = KeyCode_Space; }
        
    }
    
    if (Win32VirtualKey == VK_CAPITAL)     { Result = KeyCode_CapsLock; }
    else if (Win32VirtualKey == VK_TAB)         { Result = KeyCode_Tab; }
    else if (Win32VirtualKey == VK_LSHIFT)      { Result = KeyCode_LeftShift; }
    else if (Win32VirtualKey == VK_RSHIFT)      { Result = KeyCode_RightShift; }
    else if (Win32VirtualKey == VK_LCONTROL)    { Result = KeyCode_LeftCtrl; }
    else if (Win32VirtualKey == VK_RCONTROL)    { Result = KeyCode_RightCtrl; }
    
    // TODO(Peter): support the function key?
    //else if (Win32VirtualKey == VK_) { Result = KeyCode_Fn; }
    
    else if (Win32VirtualKey == VK_MENU)        { Result = KeyCode_Alt; }
    else if (Win32VirtualKey == VK_PRIOR)       { Result = KeyCode_PageUp; }
    else if (Win32VirtualKey == VK_NEXT)        { Result = KeyCode_PageDown; }
    else if (Win32VirtualKey == VK_BACK)        { Result = KeyCode_Backspace; }
    else if (Win32VirtualKey == VK_DELETE)      { Result = KeyCode_Delete; }
    else if (Win32VirtualKey == VK_RETURN)      { Result = KeyCode_Enter; }
    
    else if (Win32VirtualKey == VK_F1)  { Result = KeyCode_F1; }
    else if (Win32VirtualKey == VK_F2)  { Result = KeyCode_F2; }
    else if (Win32VirtualKey == VK_F3)  { Result = KeyCode_F3; }
    else if (Win32VirtualKey == VK_F4)  { Result = KeyCode_F4; }
    else if (Win32VirtualKey == VK_F5)  { Result = KeyCode_F5; }
    else if (Win32VirtualKey == VK_F6)  { Result = KeyCode_F6; }
    else if (Win32VirtualKey == VK_F7)  { Result = KeyCode_F7; }
    else if (Win32VirtualKey == VK_F8)  { Result = KeyCode_F8; }
    else if (Win32VirtualKey == VK_F9)  { Result = KeyCode_F9; }
    else if (Win32VirtualKey == VK_F10) { Result = KeyCode_F10; }
    else if (Win32VirtualKey == VK_F11) { Result = KeyCode_F11; }
    else if (Win32VirtualKey == VK_F12) { Result = KeyCode_F12; }
    
    if (!TranslateToChar)
    {
        if      (Win32VirtualKey == 0x30) { Result = KeyCode_0; }
        else if (Win32VirtualKey == 0x31) { Result = KeyCode_1; }
        else if (Win32VirtualKey == 0x32) { Result = KeyCode_2; }
        else if (Win32VirtualKey == 0x33) { Result = KeyCode_3; }
        else if (Win32VirtualKey == 0x34) { Result = KeyCode_4; }
        else if (Win32VirtualKey == 0x35) { Result = KeyCode_5; }
        else if (Win32VirtualKey == 0x36) { Result = KeyCode_6; }
        else if (Win32VirtualKey == 0x37) { Result = KeyCode_7; }
        else if (Win32VirtualKey == 0x38) { Result = KeyCode_8; }
        else if (Win32VirtualKey == 0x39) { Result = KeyCode_9; }
        
        else if (Win32VirtualKey == 0x41) { Result = KeyCode_A; }
        else if (Win32VirtualKey == 0x42) { Result = KeyCode_B; }
        else if (Win32VirtualKey == 0x43) { Result = KeyCode_C; }
        else if (Win32VirtualKey == 0x44) { Result = KeyCode_D; }
        else if (Win32VirtualKey == 0x45) { Result = KeyCode_E; }
        else if (Win32VirtualKey == 0x46) { Result = KeyCode_F; }
        else if (Win32VirtualKey == 0x47) { Result = KeyCode_G; }
        else if (Win32VirtualKey == 0x48) { Result = KeyCode_H; }
        else if (Win32VirtualKey == 0x49) { Result = KeyCode_I; }
        else if (Win32VirtualKey == 0x4A) { Result = KeyCode_J; }
        else if (Win32VirtualKey == 0x4B) { Result = KeyCode_K; }
        else if (Win32VirtualKey == 0x4C) { Result = KeyCode_L; }
        else if (Win32VirtualKey == 0x4D) { Result = KeyCode_M; }
        else if (Win32VirtualKey == 0x4E) { Result = KeyCode_N; }
        else if (Win32VirtualKey == 0x4F) { Result = KeyCode_O; }
        else if (Win32VirtualKey == 0x50) { Result = KeyCode_P; }
        else if (Win32VirtualKey == 0x51) { Result = KeyCode_Q; }
        else if (Win32VirtualKey == 0x52) { Result = KeyCode_R; }
        else if (Win32VirtualKey == 0x53) { Result = KeyCode_S; }
        else if (Win32VirtualKey == 0x54) { Result = KeyCode_T; }
        else if (Win32VirtualKey == 0x55) { Result = KeyCode_U; }
        else if (Win32VirtualKey == 0x56) { Result = KeyCode_V; }
        else if (Win32VirtualKey == 0x57) { Result = KeyCode_W; }
        else if (Win32VirtualKey == 0x58) { Result = KeyCode_X; }
        else if (Win32VirtualKey == 0x59) { Result = KeyCode_Y; }
        else if (Win32VirtualKey == 0x5A) { Result = KeyCode_Z; }
    }
    
    if (NumpadValid)
    {
        if      (Win32VirtualKey == VK_NUMPAD0) { Result = KeyCode_Num0; }
        else if (Win32VirtualKey == VK_NUMPAD1) { Result = KeyCode_Num1; }
        else if (Win32VirtualKey == VK_NUMPAD2) { Result = KeyCode_Num2; }
        else if (Win32VirtualKey == VK_NUMPAD3) { Result = KeyCode_Num3; }
        else if (Win32VirtualKey == VK_NUMPAD4) { Result = KeyCode_Num4; }
        else if (Win32VirtualKey == VK_NUMPAD5) { Result = KeyCode_Num5; }
        else if (Win32VirtualKey == VK_NUMPAD6) { Result = KeyCode_Num6; }
        else if (Win32VirtualKey == VK_NUMPAD7) { Result = KeyCode_Num7; }
        else if (Win32VirtualKey == VK_NUMPAD8) { Result = KeyCode_Num8; }
        else if (Win32VirtualKey == VK_NUMPAD9) { Result = KeyCode_Num9; }
    }
    
    if (Win32VirtualKey == VK_UP)    { Result = KeyCode_UpArrow; }
    else if (Win32VirtualKey == VK_DOWN)  { Result = KeyCode_DownArrow; }
    else if (Win32VirtualKey == VK_LEFT)  { Result = KeyCode_LeftArrow; }
    else if (Win32VirtualKey == VK_RIGHT) { Result = KeyCode_RightArrow; }
    
    return Result;
}

internal handle_window_msg_result
HandleWindowsMessage (
                      HWND WindowHandle,
                      MSG Message)
{
    handle_window_msg_result Result = {};
    Result.NeedsUpdate = 0;
    
    switch (Message.message)
    {
        case WM_HOTKEY:
        {
        }break;
        
        case WM_MOUSEWHEEL:
        {
            int MouseWheel = GET_WHEEL_DELTA_WPARAM(Message.wParam);
            /*
            Input.New->MouseScroll = MouseWheel;
            Result.NeedsUpdate = true;
            */
        }break;
        
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            /*
            Input.New->KeyStates[KeyCode_MouseLeftButton]   = (GetKeyState(VK_LBUTTON) & (1 << 15)) != 0;
            Input.New->KeyStates[KeyCode_MouseMiddleButton] = (GetKeyState(VK_MBUTTON) & (1 << 15)) != 0;
            Input.New->KeyStates[KeyCode_MouseRightButton]  = (GetKeyState(VK_RBUTTON) & (1 << 15)) != 0;
            // NOTE(Peter): If you decide to support extra mouse buttons, on windows the key codes are
            // VK_XBUTTON1 and VK_XBUTTON2
            
            if (KeyTransitionedDown(KeyCode_MouseLeftButton, Input))
            {
                Input.MouseDownX = Input.New->MouseX;
                Input.MouseDownY = Input.New->MouseY;
            }
            Result.NeedsUpdate = true;*/
        }break;
        
        case WM_MOUSEMOVE:
        {
            POINT MousePos;
            GetCursorPos(&MousePos);
            ScreenToClient(WindowHandle, &MousePos);
            
            /*
            Input.New->MouseX = MousePos.x;
            Input.New->MouseY = App.WindowHeight - MousePos.y;
            
            Result.NeedsUpdate = true;
            */
        }break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            int VirtualKey = (int)Message.wParam;
            bool KeyDown = (Message.lParam & (1 << 31)) == 0;
            int KeyIndex = Win32GetKeyCode(VirtualKey, true, true);
            /*
            if (KeyIndex >= 0)
            {
                DEBUG_GET_MESSAGE_NAME(Result.MessageType, "WM_KEYEvent ");
                Input.New->KeyStates[KeyIndex] = KeyDown;
                Result.NeedsUpdate = true;
            }
            else
            {
                if (Input.TranslateInputToCharValues && KeyDown)
                {
                    // NOTE(Peter): Took this out b/c we're translating the WM_CHAR messages
                    // in the message pump, and if we do it here as well, character producing
                    // key messages get put on the message queue twice
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    DEBUG_GET_MESSAGE_NAME(Result.MessageType, "WM_KEYEvent ");
                    // NOTE(Peter): This is so that when you lift up a key that was generating a WM_CHAR,
                    // the app still has a chance to respond to it.
                    Result.NeedsUpdate = true;
                }
            }
            */
        }break;
        
        case WM_CHAR:
        {
            /*
            char TranslatedChar = (char)Message.wParam;
            int KeyIndex = GetKeyIndexFromChar(TranslatedChar);
            
            if (KeyIndex >= 0)
            {
                // NOTE(Peter): Always setting this to true becuase windows is stupid and doesn't
                // pass the press/release bit through correctly. So now the KEYDOWN/KEYUP Messages above
                // only translate the message to a WM_CHAR message if its a key down. Since we clear all
                // keystates to false at the beginning of an input frame, this will make transitions
                // get registered correctly.
                Input.New->KeyStates[KeyIndex] = true;
                Result.NeedsUpdate = true;
            }
            else
            {
                printf("Translated Char Not Recognized: %c\n", TranslatedChar);
            }
            */
        }break;
        
        default:
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }break;
    }
    
    return Result;
}

internal void
Win32UpdateWindowDimension(window* Window)
{
    RECT ClientRect;
    GetClientRect(Window->Handle, &ClientRect);
    Window->Width = ClientRect.right - ClientRect.left;
    Window->Height = ClientRect.bottom - ClientRect.top;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Win32Buffer, int Width, int Height)
{
    if(Win32Buffer->Buffer.Memory)
    {
        VirtualFree(Win32Buffer->Buffer.Memory, 0, MEM_RELEASE);
    }
    
    Win32Buffer->Buffer.Width = Width;
    Win32Buffer->Buffer.Height = Height;
    
    int BytesPerPixel = 4;
    Win32Buffer->Buffer.BytesPerPixel = BytesPerPixel;
    
    Win32Buffer->Info.bmiHeader.biSize = sizeof(Win32Buffer->Info.bmiHeader);
    Win32Buffer->Info.bmiHeader.biWidth = Win32Buffer->Buffer.Width;
    Win32Buffer->Info.bmiHeader.biHeight = -Win32Buffer->Buffer.Height; // Top down, not bottom up
    Win32Buffer->Info.bmiHeader.biPlanes = 1;
    Win32Buffer->Info.bmiHeader.biBitCount = 32;
    Win32Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int BitmapMemorySize = (Win32Buffer->Buffer.Width * Win32Buffer->Buffer.Height)*BytesPerPixel;
    Win32Buffer->Buffer.Memory = (u8*)VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Win32Buffer->Buffer.Pitch = Width*BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer* Win32Buffer, window Window)
{
    StretchDIBits(Window.DeviceContext,
                  0, 0, Win32Buffer->Buffer.Width, Win32Buffer->Buffer.Height,
                  0, 0, Win32Buffer->Buffer.Width, Win32Buffer->Buffer.Height,
                  Win32Buffer->Buffer.Memory,
                  &Win32Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

/////////////////////////////////////////
//
//            Open GL
//
/////////////////////////////////////////


internal void
OpenGLRenderTriBuffer (u8* Vertecies, s32 VertexElements,
                       u8* UVs, s32 UVElements,
                       u8* Colors, s32 ColorsElements,
                       s32 TriCount)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(VertexElements, GL_FLOAT, VertexElements * sizeof(r32), Vertecies);
    
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(UVElements, GL_FLOAT, UVElements * sizeof(r32), UVs);
    
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(ColorsElements, GL_FLOAT, ColorsElements * sizeof(r32), Colors);
    
    glDrawArrays(GL_TRIANGLES, 0, TriCount);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

internal void
OpenGLDraw3DTri (v4 P0, v4 P1, v4 P2,
                 v2 UV0, v2 UV1, v2 UV2,
                 v4 C0, v4 C1, v4 C2)
{
    glBegin(GL_TRIANGLES);
    
    glTexCoord2f(UV0.x, UV0.y);
    glColor4f(C0.r, C0.g, C0.b, C0.a);
    glVertex4f(P0.x, P0.y, P0.z, P0.w);
    
    glTexCoord2f(UV1.x, UV1.y);
    glColor4f(C1.r, C1.g, C1.b, C1.a);
    glVertex4f(P1.x, P1.y, P1.z, P1.w);
    
    glTexCoord2f(UV2.x, UV2.y);
    glColor4f(C2.r, C2.g, C2.b, C2.a);
    glVertex4f(P2.x, P2.y, P2.z, P2.w);
    
    glEnd();
}

internal void
OpenGLDraw2DTri (v2 P0, v2 P1, v2 P2,
                 v2 UV0, v2 UV1, v2 UV2,
                 v4 C0, v4 C1, v4 C2)
{
    glBegin(GL_TRIANGLES);
    
    glTexCoord2f(UV0.x, UV0.y);
    glColor4f(C0.r, C0.g, C0.b, C0.a);
    glVertex2f(P0.x, P0.y);
    
    glTexCoord2f(UV1.x, UV1.y);
    glColor4f(C1.r, C1.g, C1.b, C1.a);
    glVertex2f(P1.x, P1.y);
    
    glTexCoord2f(UV2.x, UV2.y);
    glColor4f(C2.r, C2.g, C2.b, C2.a);
    glVertex2f(P2.x, P2.y);
    
    glEnd();
}

internal void
LoadModelView (r32 Matrix[16])
{
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(Matrix);
}

internal void
LoadProjection (r32 Matrix[16])
{
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(Matrix);
}

internal void
ClearRenderBuffer ()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

s32 NextTextureHandle = 1;
internal s32
SubmitTexture (u8* Memory, s32 Width, s32 Height)
{
    s32 TextureHandle = NextTextureHandle++;
    glBindTexture(GL_TEXTURE_2D, TextureHandle);
    glTexImage2D(GL_TEXTURE_2D,
                 0, // mip map level
                 GL_RGBA8,
                 Width,
                 Height,
                 0, // border
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 Memory);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    return TextureHandle;
}

internal void
BindTexture (s32 TextureHandle)
{
    glBindTexture(GL_TEXTURE_2D, TextureHandle);
}

#define GS_WIN32_CPP
#endif // GS_WIN32_CPP

#define GS_WIN32_CPP
#endif // GS_WIN32_CPP