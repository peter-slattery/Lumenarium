PANEL_INIT_PROC(ProfilerView_Init)
{
    
}

PANEL_CLEANUP_PROC(ProfilerView_Cleanup)
{
    
}

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

PANEL_RENDER_PROC(ProfilerView_Render)
{
    memory_arena* Memory = &State->Transient;
    string String = InitializeEmptyString(PushArray(Memory, char, 256), 256);
    
    v4 FrameColors[] = { GreenV4, YellowV4, RedV4, WhiteV4 };
    
    r32 FrameListHeight = 64;
    v2 FrameListMin = v2{PanelBounds.Min.x + 16, PanelBounds.Max.y - (16 + FrameListHeight)};
    v2 FrameListMax = v2{PanelBounds.Max.x - 16, PanelBounds.Max.y - 16};
    
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
    DrawString(RenderBuffer, String, State->Interface.Font, FrameListMin - v2{0, 32}, WhiteV4);
    
    v2 ButtonMin = v2{FrameListMax.x - 128, FrameListMin.y - 32};
    v2 ButtonMax = ButtonMin + v2{128, 28};
    button_result ShouldResumeRecording = EvaluateButton(RenderBuffer, ButtonMin, ButtonMax,
                                                         MakeString("Resume Recording"), State->Interface, Mouse);
    if (ShouldResumeRecording.Pressed)
    {
        GlobalDebugServices->RecordFrames = true;
    }
    
    ButtonMin = v2{FrameListMin.x, FrameListMin.y - 60};
    ButtonMax = v2{FrameListMin.x + 128, FrameListMin.y - 42};
    button_result ActivateScopeView = EvaluateButton(RenderBuffer, ButtonMin, ButtonMax,
                                                     MakeString("Scope View"), State->Interface, Mouse);
    
    ButtonMin.x += 152;
    ButtonMax.x += 152;
    button_result ActivateListView = EvaluateButton(RenderBuffer, ButtonMin, ButtonMax,
                                                    MakeString("List View"), State->Interface, Mouse);
    
    if (ActivateScopeView.Pressed) { GlobalDebugServices->Interface.FrameView = FRAME_VIEW_PROFILER; }
    if (ActivateListView.Pressed) { GlobalDebugServices->Interface.FrameView = FRAME_VIEW_SCOPE_LIST; }
    
    v2 ViewModeMin = v2{FrameListMin.x, PanelBounds.Min.y};
    v2 ViewModeMax = v2{FrameListMax.x, FrameListMin.y - 96};
    
    if (GlobalDebugServices->Interface.FrameView == FRAME_VIEW_PROFILER)
    {
        RenderProfiler_ScopeVisualization(RenderBuffer, State->Interface, Mouse, 
                                          ViewModeMin, ViewModeMax,
                                          VisibleFrame, Memory);
    }
    else
    {
        RenderProfiler_ListVisualization(RenderBuffer, State->Interface, Mouse,
                                         ViewModeMin, ViewModeMax,
                                         VisibleFrame, Memory);
    }
}