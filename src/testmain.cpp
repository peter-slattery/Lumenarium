#include <windows.h>
#include "gs_platform.h"
#include "gs_win32.h"

int main (int ArgCount, char** Args)
{
    window MainWindow = PlatformCreateWindow("Test Window", 1024, 768);
    
    win32_opengl_window_info OpenGLInfo = {};
    OpenGLInfo.ColorBits = 32;
    OpenGLInfo.AlphaBits = 8;
    Win32CreateOpenGLContext(OpenGLInfo, MainWindow);
    
    
    return 0;
}