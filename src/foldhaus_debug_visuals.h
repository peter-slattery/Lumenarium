internal void
DrawDebugInterface (render_command_buffer* RenderBuffer, r32 StartX, interface_config Interface, r32 WindowWidth, r32 WindowHeight, r32 DeltaTime, app_state* State, camera Camera, mouse_state Mouse, memory_arena* Transient)
{
    DEBUG_TRACK_SCOPE(DrawDebugInterface);
    
    v2 TopOfDebugView = v2{StartX, WindowHeight - (NewLineYOffset(*Interface.Font) + 5)};
    v2 TopOfScreenLinePos = TopOfDebugView;
    
    arena_snapshot StartTempMemory = TakeSnapshotOfArena(*Transient);
    
    string DebugString = InitializeEmptyString(PushArray(Transient, char, 256), 256);
    
    if (GlobalDebugServices->Interface.ShowCameraMouse || GlobalDebugServices->Interface.ShowTrackedScopes)
    {
        PushRenderQuad2D(RenderBuffer, 
                         v2{TopOfDebugView.x, TopOfDebugView.y - 500},
                         v2{TopOfDebugView.x + 700, TopOfDebugView.y},
                         v4{0, 0, 0, .8f});
    }
    
    r32 FramesPerSecond = 1.0f / DeltaTime;
    
    string ModeName = MakeStringLiteral("Base Mode");
    if (State->Modes.ActiveModesCount > 0 && 
        State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Name.Length > 0)
    {
        ModeName = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Name;
    }
    
    PrintF(&DebugString, "Framerate: %.*f s   %d fps    |   Current Mode: %.*s  Modes: %d  Memory Used: %d / %d    |   Commands: %d",
           5, DeltaTime,
           (u32)FramesPerSecond,
           ModeName.Length, ModeName.Memory,
           State->Modes.ActiveModesCount,
           State->Modes.Arena.CurrentRegion->Used,
           State->Modes.Arena.CurrentRegion->Size,
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
        
        s32 MousePrecision = 0;
        PrintF(&DebugString, "Mouse Pos: (%.*f, %.*f) Down: (%.*f, %.*f)",
               MousePrecision, Mouse.Pos.x,
               MousePrecision, Mouse.Pos.y,
               MousePrecision, Mouse.DownPos.x,
               MousePrecision, Mouse.DownPos.y);
        DrawString(RenderBuffer, DebugString, Interface.Font,
                   TopOfScreenLinePos, WhiteV4);
        TopOfScreenLinePos.y -= NewLineYOffset(*Interface.Font);
        
        PrintF(&DebugString, "Nodes Memory: %d / %d",
               State->NodeList->TotalUsed,
               State->NodeList->TotalMax);
        DrawString(RenderBuffer, DebugString, Interface.Font,
                   TopOfScreenLinePos, WhiteV4);
        TopOfScreenLinePos.y -= NewLineYOffset(*Interface.Font);
    }
    
    if (GlobalDebugServices->Interface.ShowTrackedScopes)
    {
        r32 ColumnsStartX = TopOfScreenLinePos.x;
        
        for (s32 i = 0; i < GlobalDebugServices->ScopeHistogramUsed; i++)
        {
            v2 Register = v2{ColumnsStartX, TopOfScreenLinePos.y};
            
            s32 CurrentFrame = GlobalDebugServices->ScopeHistogramSorted[i].CurrentFrame - 1;
            if (CurrentFrame < 0) { CurrentFrame = HISTOGRAM_DEPTH - 1; }
            
            u64 CyclesPerHit = GlobalDebugServices->ScopeHistogramSorted[i].PerFrame_Cycles[CurrentFrame];
            r32 SecondsPerHit = (r32)CyclesPerHit / (r32)GlobalDebugServices->PerformanceCountFrequency;
            
            // Column 1
            PrintF(&DebugString, "%.*s",
                   GlobalDebugServices->ScopeHistogramSorted[i].ScopeName.Length,
                   GlobalDebugServices->ScopeHistogramSorted[i].ScopeName.Memory);
            r32 ColumnOneX = DrawString(RenderBuffer, DebugString, Interface.Font, 
                                        Register, WhiteV4).x;
            Register.x += GSMax(ColumnOneX - Register.x, 250.f);
            
            // Column 2
            PrintF(&DebugString, "%d hits", GlobalDebugServices->ScopeHistogramSorted[i].PerFrame_CallCount[CurrentFrame]);
            r32 ColumnTwoX = DrawString(RenderBuffer, DebugString, Interface.Font, 
                                        Register, WhiteV4).x;
            Register.x += GSMax(ColumnTwoX - Register.x, 150.f);
            
            // Column 3
            PrintF(&DebugString, "%lld cycles", CyclesPerHit);
            r32 ColumnThreeX = DrawString(RenderBuffer, DebugString, Interface.Font, 
                                          Register, WhiteV4).x;
            Register.x += GSMax(ColumnThreeX - Register.x, 200.f);
            
            PrintF(&DebugString, "%f sec", SecondsPerHit);
            r32 ColumnFourX = DrawString(RenderBuffer, DebugString, Interface.Font, 
                                         Register, WhiteV4).x;
            Register.x += GSMax(ColumnFourX - Register.x, 200.f);
            
            TopOfScreenLinePos.y -= NewLineYOffset(*Interface.Font);
            
            
        }
    }
    
    ZeroArenaToSnapshot(Transient, StartTempMemory);
    ClearArenaToSnapshot(Transient, StartTempMemory);
}
