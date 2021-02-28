//
// File: win32_mouse.h
// Author: Peter Slattery
// Creation Date: 2021-01-10
//
#ifndef WIN32_MOUSE_H

HCURSOR CursorArrow;
HCURSOR CursorPointer;
HCURSOR CursorLoading;
HCURSOR CursorHArrows;
HCURSOR CursorVArrows;
HCURSOR CursorDTopLeftArrows;
HCURSOR CursorDTopRightArrows;

HCURSOR CurrentCursor;

// NOTE(Peter): Only meant to take one of the values specified below:
// IDC_APPSTARTING, IDC_ARROW, IDC_CROSS, IDC_HAND, IDC_HELP, IDC_IBEAM,
// IDC_ICON, IDC_NO, IDC_SIZE, IDC_SIZEALL, IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE,
// IDC_SIZEWE, IDC_UPARROW, IDC_WAIT
internal HCURSOR
Win32LoadSystemCursor(char* CursorIdentifier)
{
    u32 Error = 0;
    HCURSOR Result = LoadCursorA(NULL, CursorIdentifier);
    if (Result == NULL)
    {
        Error = GetLastError();
        InvalidCodePath;
    }
    return Result;
}

internal void
Mouse_Init()
{
    CursorArrow = Win32LoadSystemCursor(IDC_ARROW);
    CursorPointer = Win32LoadSystemCursor(IDC_HAND);
    CursorLoading = Win32LoadSystemCursor(IDC_WAIT);
    CursorHArrows = Win32LoadSystemCursor(IDC_SIZEWE);
    CursorVArrows = Win32LoadSystemCursor(IDC_SIZENS);
    CursorDTopLeftArrows = Win32LoadSystemCursor(IDC_SIZENWSE);
    CursorDTopRightArrows = Win32LoadSystemCursor(IDC_SIZENESW);
}

internal void
Mouse_Update(window Window, context* Context)
{
    POINT Pos;
    GetCursorPos (&Pos);
    ScreenToClient(Window.Handle, &Pos);
    
    Context->Mouse.Scroll = 0;
    Context->Mouse.OldPos = Context->Mouse.Pos;
    Context->Mouse.Pos = v2{(r32)Pos.x, (r32)Window.Height - Pos.y};
    Context->Mouse.DeltaPos = Context->Mouse.Pos - Context->Mouse.OldPos;
    
    if (KeyIsDown(Context->Mouse.LeftButtonState))
    {
        SetKeyWasDown(Context->Mouse.LeftButtonState);
    }
    else
    {
        SetKeyWasUp(Context->Mouse.LeftButtonState);
    }
    
    if (KeyIsDown(Context->Mouse.MiddleButtonState))
    {
        SetKeyWasDown(Context->Mouse.MiddleButtonState);
    }
    else
    {
        SetKeyWasUp(Context->Mouse.MiddleButtonState);
    }
    
    
    if (KeyIsDown(Context->Mouse.RightButtonState))
    {
        SetKeyWasDown(Context->Mouse.RightButtonState);
    }
    else
    {
        SetKeyWasUp(Context->Mouse.RightButtonState);
    }
}


internal void
Mouse_Advance(context* Context)
{
    Context->Mouse.LeftButtonState = GetMouseButtonStateAdvanced(Context->Mouse.LeftButtonState);
    Context->Mouse.MiddleButtonState = GetMouseButtonStateAdvanced(Context->Mouse.MiddleButtonState);
    Context->Mouse.RightButtonState = GetMouseButtonStateAdvanced(Context->Mouse.RightButtonState);
    
    HCURSOR NewCursor = 0;
    switch (Context->Mouse.CursorType)
    {
        case CursorType_Arrow:   { NewCursor = CursorArrow;  } break;
        case CursorType_Pointer: { NewCursor = CursorPointer; }break;
        case CursorType_Loading: { NewCursor = CursorLoading; }break;
        case CursorType_HArrows: { NewCursor = CursorHArrows; }break;
        case CursorType_VArrows: { NewCursor = CursorVArrows; }break;
        case CursorType_DTopLeftArrows: { NewCursor = CursorDTopLeftArrows; }break;
        case CursorType_DTopRightArrows: { NewCursor = CursorDTopRightArrows; }break;
        
        InvalidDefaultCase;
    }
    if (NewCursor != CurrentCursor)
    {
        CurrentCursor = NewCursor;
        SetCursor(NewCursor);
    }
}


#define WIN32_MOUSE_H
#endif // WIN32_MOUSE_H