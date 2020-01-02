//
// File: foldhaus_debug_visuals.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_DEBUG_VISUALS_H

internal void
DrawDebugInterface (render_command_buffer* RenderBuffer, r32 StartX, interface_config Interface, r32 WindowWidth, r32 WindowHeight, r32 DeltaTime, app_state* State, camera Camera, mouse_state Mouse, memory_arena* Transient)
{
    DEBUG_TRACK_SCOPE(DrawDebugInterface);
    
    v2 TopOfDebugView = v2{StartX, WindowHeight - (NewLineYOffset(*Interface.Font) + 5)};
    v2 TopOfScreenLinePos = TopOfDebugView;
    
    string DebugString = InitializeEmptyString(PushArray(Transient, char, 256), 256);
    
    if (GlobalDebugServices->Interface.ShowCameraMouse)
    {
        PushRenderQuad2D(RenderBuffer, 
                         v2{TopOfDebugView.x, TopOfDebugView.y - 500},
                         v2{TopOfDebugView.x + 700, TopOfDebugView.y},
                         v4{0, 0, 0, .8f});
    }
    
    r32 FramesPerSecond = 1.0f / DeltaTime;
    
    PrintF(&DebugString, "Framerate: %.*f s   %d fps    |   Modes: %d  Memory Used: %d / %d    |   Commands: %d   |   HI SAM!!!!  ",
           5, DeltaTime,
           (u32)FramesPerSecond,
           State->Modes.ActiveModesCount,
           State->Modes.Arena.TotalUsed,
           State->Modes.Arena.TotalSize,
           State->CommandQueue.Used);
    DrawString(RenderBuffer, DebugString, Interface.Font, TopOfScreenLinePos, WhiteV4);
    
    v2 ButtonDim = v2{200, (r32)NewLineYOffset(*Interface.Font) + 10};
    TopOfScreenLinePos.y -= ButtonDim.y + 10;
    v2 ButtonPos = TopOfScreenLinePos;
    button_result CameraBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim, 
                                             MakeStringLiteral("Camera"), Interface, Mouse);
    
    ButtonPos.x += ButtonDim.x + 10;
    button_result ScopeTimeBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim, 
                                                MakeStringLiteral("Scope Time"), Interface, Mouse);
    ButtonPos.x += ButtonDim.x + 10;
    button_result RenderSculptureBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim,
                                                      MakeStringLiteral("Visualize"), Interface, Mouse);
    
    ButtonPos.x += ButtonDim.x + 10;
    
    string SACNButtonString;
    if (GlobalDebugServices->Interface.SendSACNData)
    {
        SACNButtonString = MakeStringLiteral("Turn SACN Off");
    }
    else
    {
        SACNButtonString = MakeStringLiteral("Turn SACN On");
    }
    
    button_result SendSACNDataBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim,
                                                   SACNButtonString, Interface, Mouse);
    
    TopOfScreenLinePos.y -= NewLineYOffset(*Interface.Font) + 10;
    
    if (CameraBtn.Pressed)
    {
        GlobalDebugServices->Interface.ShowCameraMouse = !GlobalDebugServices->Interface.ShowCameraMouse;
    }
    
    if (ScopeTimeBtn.Pressed)
    {
        GlobalDebugServices->Interface.ShowTrackedScopes = !GlobalDebugServices->Interface.ShowTrackedScopes;
    }
    
    if (RenderSculptureBtn.Pressed)
    {
        GlobalDebugServices->Interface.RenderSculpture = 
            !GlobalDebugServices->Interface.RenderSculpture;
    }
    
    if (SendSACNDataBtn.Pressed)
    {
        GlobalDebugServices->Interface.SendSACNData = !GlobalDebugServices->Interface.SendSACNData;
    }
    
    if (GlobalDebugServices->Interface.ShowCameraMouse)
    {
        PrintF(&DebugString, "Camera x=%.*f y=%.*f z=%.*f   LookAt x=%.*f y=%.*f z=%.*f",
               3, Camera.Position.x,
               3, Camera.Position.y,
               3, Camera.Position.z,
               3, Camera.LookAt.x,
               3, Camera.LookAt.y,
               3, Camera.LookAt.z);
        DrawString(RenderBuffer, DebugString, Interface.Font, TopOfScreenLinePos, v4{1.0f, 1.0f, 1.0f, 1.0f});
        TopOfScreenLinePos.y -= NewLineYOffset(*Interface.Font);
        
        b32 LeftButtonIsDown = (Mouse.LeftButtonState & KeyState_IsDown) > 0;
        b32 LeftButtonWasDown = (Mouse.LeftButtonState & KeyState_WasDown) > 0;
        
        s32 MousePrecision = 0;
        PrintF(&DebugString, "Mouse Pos: (%.*f, %.*f) Down: (%.*f, %.*f) State: %s %s",
               MousePrecision, Mouse.Pos.x,
               MousePrecision, Mouse.Pos.y,
               MousePrecision, Mouse.DownPos.x,
               MousePrecision, Mouse.DownPos.y,
               (LeftButtonIsDown ? "Is Down" : "Is Not Down"),
               (LeftButtonWasDown ? "Was Down" : "Was Not Down"));
        DrawString(RenderBuffer, DebugString, Interface.Font,
                   TopOfScreenLinePos, WhiteV4);
        TopOfScreenLinePos.y -= NewLineYOffset(*Interface.Font);
        
        PrintF(&DebugString, "Render Buffer: %d / %d (at this point)",
               RenderBuffer->CommandMemoryUsed,
               RenderBuffer->CommandMemorySize);
        DrawString(RenderBuffer, DebugString, Interface.Font,
                   TopOfScreenLinePos, WhiteV4);
        TopOfScreenLinePos.y -= NewLineYOffset(*Interface.Font);
    }
}


#define FOLDHAUS_DEBUG_VISUALS_H
#endif // FOLDHAUS_DEBUG_VISUALS_H