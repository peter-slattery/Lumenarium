
internal void
RenderProfiler_ScopeVisualization(render_command_buffer* RenderBuffer,
                                  interface_config Interface, mouse_state Mouse,
                                  v2 Min, v2 Max, 
                                  debug_frame* VisibleFrame, memory_arena* Memory)
{
    v4 ThreadColors[] = {
        v4{.73f, .33f, .83f, 1},
        v4{0,   .50f, .50f, 1},
        v4{.83f, 0,    0, 1},
        v4{.33f, .49f, .83f, 1},
        v4{.74f, .40f, .25f, 1},
    };
    
    r32 Width = Max.x - Min.x;
    r32 DepthHeight = 64;
    
    s64 FrameStartCycles = VisibleFrame->FrameStartCycles;
    s64 FrameTotalCycles = VisibleFrame->FrameEndCycles - VisibleFrame->FrameStartCycles;
    
    debug_scope_record_list* ThreadScopeCalls = GetScopeListForThreadInFrame(GlobalDebugServices,
                                                                             VisibleFrame);
    
    MakeStringBuffer(String, 256);
    for (s32 i = 0; i < ThreadScopeCalls->Count; i++)
    {
        scope_record* Record = ThreadScopeCalls->Calls + i;
        scope_name* Name = GetOrAddNameHashEntry(VisibleFrame, Record->NameHash);
        r32 PercentStart = (r32)(Record->StartCycles - FrameStartCycles) / (r32)FrameTotalCycles;
        r32 PercentEnd = (r32)(Record->EndCycles - FrameStartCycles) / (r32)FrameTotalCycles;
        
        v2 ScopeMin = v2{Min.x + (Width * PercentStart), Max.y - ((Record->CallDepth + 1) * DepthHeight)};
        v2 ScopeMax = v2{Min.x + (Width * PercentEnd), ScopeMin.y + (DepthHeight - 4)};
        
        if ((ScopeMax.x - ScopeMin.x) >= 1)
        {
            v4 Color = ThreadColors[0];
            if (PointIsInRange(Mouse.Pos, ScopeMin, ScopeMax))
            {
                Color = GreenV4;
            }
            
            PushRenderQuad2D(RenderBuffer, ScopeMin, ScopeMax, Color);
            PushRenderBoundingBox2D(RenderBuffer, ScopeMin, ScopeMax, 1, BlackV4);
            
            if (PointIsInRange(Mouse.Pos, ScopeMin, ScopeMax))
            {
                PushRenderQuad2D(RenderBuffer, Mouse.Pos, Mouse.Pos + v2{256, 32}, BlackV4);
                PrintF(&String, "%.*s : %d - %d", Name->Name.Length, Name->Name.Memory, Record->StartCycles, Record->EndCycles);
                DrawString(RenderBuffer, String, Interface.Font, Mouse.Pos, WhiteV4);
            }
        }
    }
}

internal void
RenderProfiler_ListVisualization(render_command_buffer* RenderBuffer,
                                 interface_config Interface, mouse_state Mouse,
                                 v2 Min, v2 Max, 
                                 debug_frame* VisibleFrame, memory_arena* Memory)
{
    MakeStringBuffer(String, 256);
    
    r32 YAt = Max.y - Interface.Font->PixelHeight;
    r32 Column0X = Min.x;
    r32 Column1X = Column0X + 256;
    r32 Column2X = Column1X + 128;
    r32 Column3X = Column2X + 128;
    r32 Column4X = Column3X + 100;
    
    for (s32 n = 0; n < VisibleFrame->ScopeNamesMax; n++)
    {
        scope_name NameEntry = VisibleFrame->ScopeNamesHash[n];
        if (NameEntry.Hash != 0)
        {
            collated_scope_record* CollatedRecord = VisibleFrame->CollatedScopes + n;
            
            PrintF(&String, "%.*s", NameEntry.Name.Length, NameEntry.Name.Memory);
            DrawString(RenderBuffer, String, Interface.Font, v2{Column0X, YAt}, WhiteV4);
            
            PrintF(&String, "%f", CollatedRecord->PercentFrameTime);
            DrawString(RenderBuffer, String, Interface.Font, v2{Column1X, YAt}, WhiteV4);
            
            PrintF(&String, "%fs", CollatedRecord->TotalSeconds);
            DrawString(RenderBuffer, String, Interface.Font, v2{Column2X, YAt}, WhiteV4);
            
            PrintF(&String, "%dcy", CollatedRecord->TotalCycles);
            DrawString(RenderBuffer, String, Interface.Font, v2{Column3X, YAt}, WhiteV4);
            
            PrintF(&String, "%d calls", CollatedRecord->CallCount);
            DrawString(RenderBuffer, String, Interface.Font, v2{Column4X, YAt}, WhiteV4);
            
            YAt -= Interface.Font->PixelHeight + 4;
            
            if (YAt < Min.y) { break; }
        }
    }
}

internal void
DrawDebugFrameList (render_command_buffer* RenderBuffer, interface_config Interface, mouse_state Mouse, v2 BoundsMin, v2 BoundsMax, memory_arena* Memory)
{
    string String = InitializeEmptyString(PushArray(Memory, char, 256), 256);
    
    v4 FrameColors[] = { GreenV4, YellowV4, RedV4, WhiteV4 };
    
    r32 FrameListHeight = 64;
    v2 FrameListMin = v2{BoundsMin.x + 16, BoundsMax.y - (16 + FrameListHeight)};
    v2 FrameListMax = v2{BoundsMax.x - 16, BoundsMax.y - 16};
    
    r32 FrameListPadding = 4;
    r32 FrameListInnerWidth = (FrameListMax.x - FrameListMin.x) - (FrameListPadding * 2);
    
    r32 SingleFrameStep = FrameListInnerWidth / DEBUG_FRAME_COUNT;
    r32 SingleFrameWidth = (r32)((s32)SingleFrameStep - 2);
    
    PushRenderBoundingBox2D(RenderBuffer, FrameListMin, FrameListMax, 2, WhiteV4);
    
    if (PointIsInRange(Mouse.Pos, FrameListMin, FrameListMax) &&
        MouseButtonHeldDown(Mouse.LeftButtonState))
    {
        r32 LocalMouseX = (Mouse.Pos.x - FrameListMin.x) + FrameListPadding;
        s32 ClosestFrameIndex = (LocalMouseX / SingleFrameStep);
        
        if (ClosestFrameIndex >= 0 && ClosestFrameIndex < DEBUG_FRAME_COUNT)
        {
            GlobalDebugServices->RecordFrames = false;
            GlobalDebugServices->CurrentDebugFrame = ClosestFrameIndex;
        }
    }
    
    for (s32 F = 0; F < DEBUG_FRAME_COUNT; F++)
    {
        v2 Min = v2{FrameListMin.x + FrameListPadding + (F * SingleFrameStep), FrameListMin.y + 4};
        v2 Max = v2{Min.x + SingleFrameWidth, FrameListMax.y - 4};
        
        s32 FramesAgo = (GlobalDebugServices->CurrentDebugFrame - F);
        if (FramesAgo < 0) { FramesAgo += DEBUG_FRAME_COUNT; }
        v4 Color = FrameColors[GSClamp(0, FramesAgo, 3)];
        PushRenderQuad2D(RenderBuffer, Min, Max, Color);
    }
    
    debug_frame* VisibleFrame = GetLastDebugFrame(GlobalDebugServices);
    s64 FrameStartCycles = VisibleFrame->FrameStartCycles;
    s64 FrameTotalCycles = VisibleFrame->FrameEndCycles - VisibleFrame->FrameStartCycles;
    
    PrintF(&String, "Frame %d - Total Cycles: %lld", 
           GlobalDebugServices->CurrentDebugFrame - 1, 
           FrameTotalCycles);
    DrawString(RenderBuffer, String, Interface.Font, FrameListMin - v2{0, 32}, WhiteV4);
    
    v2 ButtonMin = v2{FrameListMax.x - 128, FrameListMin.y - 32};
    v2 ButtonMax = ButtonMin + v2{128, 28};
    button_result ShouldResumeRecording = EvaluateButton(RenderBuffer, ButtonMin, ButtonMax,
                                                         MakeString("Resume Recording"), Interface, Mouse);
    if (ShouldResumeRecording.Pressed)
    {
        GlobalDebugServices->RecordFrames = true;
    }
    
    ButtonMin = v2{FrameListMin.x, FrameListMin.y - 60};
    ButtonMax = v2{FrameListMin.x + 128, FrameListMin.y - 42};
    button_result ActivateScopeView = EvaluateButton(RenderBuffer, ButtonMin, ButtonMax,
                                                     MakeString("Scope View"), Interface, Mouse);
    
    ButtonMin.x += 152;
    ButtonMax.x += 152;
    button_result ActivateListView = EvaluateButton(RenderBuffer, ButtonMin, ButtonMax,
                                                    MakeString("List View"), Interface, Mouse);
    
    if (ActivateScopeView.Pressed) { GlobalDebugServices->Interface.FrameView = FRAME_VIEW_PROFILER; }
    if (ActivateListView.Pressed) { GlobalDebugServices->Interface.FrameView = FRAME_VIEW_SCOPE_LIST; }
    
    v2 ViewModeMin = v2{FrameListMin.x, BoundsMin.y};
    v2 ViewModeMax = v2{FrameListMax.x, FrameListMin.y - 96};
    
    if (GlobalDebugServices->Interface.FrameView == FRAME_VIEW_PROFILER)
    {
        RenderProfiler_ScopeVisualization(RenderBuffer, Interface, Mouse, 
                                          ViewModeMin, ViewModeMax,
                                          VisibleFrame, Memory);
    }
    else
    {
        RenderProfiler_ListVisualization(RenderBuffer, Interface, Mouse,
                                         ViewModeMin, ViewModeMax,
                                         VisibleFrame, Memory);
    }
}

internal void
DrawDebugInterface (render_command_buffer* RenderBuffer, r32 StartX, interface_config Interface, r32 WindowWidth, r32 WindowHeight, r32 DeltaTime, app_state* State, camera Camera, mouse_state Mouse, memory_arena* Transient)
{
    DEBUG_TRACK_SCOPE(DrawDebugInterface);
    
    v2 TopOfDebugView = v2{StartX, WindowHeight - (NewLineYOffset(*Interface.Font) + 5)};
    v2 TopOfScreenLinePos = TopOfDebugView;
    
    arena_snapshot StartTempMemory = TakeSnapshotOfArena(*Transient);
    
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
    
    if (GlobalDebugServices->Interface.ShowTrackedScopes)
    {
        v2 ProfilerMin = v2{TopOfDebugView.x, TopOfDebugView.y - 500};
        v2 ProfilerMax = v2{TopOfDebugView.x + 700, TopOfDebugView.y - 64};
        PushRenderQuad2D(RenderBuffer, ProfilerMin, ProfilerMax, v4{0, 0, 0, .8f});
        DrawDebugFrameList(RenderBuffer, Interface, Mouse, ProfilerMin, ProfilerMax, Transient);
        
#if 0
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
#endif
    }
    
    ZeroArenaToSnapshot(Transient, StartTempMemory);
    ClearArenaToSnapshot(Transient, StartTempMemory);
}
