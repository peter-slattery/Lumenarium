//
// File: foldhaus_panel_profiler.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_PROFILER_H

input_command ProfilerView_Commands[] = {{}};
s32 ProfilerView_CommandsCount = 0;

GSMetaTag(panel_init);
GSMetaTag(panel_type_profiler);
internal void
ProfilerView_Init(panel* Panel, app_state* State, context Context)
{
    
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_profiler);
internal void
ProfilerView_Cleanup(panel* Panel, app_state* State)
{
    
}

internal void
RenderProfiler_ScopeVisualization(ui_interface* Interface, ui_widget* Layout, debug_frame* VisibleFrame, gs_memory_arena* Memory)
{
    v4 ThreadColors[] = {
        v4{.73f, .33f, .83f, 1},
        v4{0,   .50f, .50f, 1},
        v4{.83f, 0,    0, 1},
        v4{.33f, .49f, .83f, 1},
        v4{.74f, .40f, .25f, 1},
    };
    
    rect2 Bounds = ui_LayoutRemaining(*Layout);
    r32 Width = Rect2Width(Bounds);
    r32 DepthHeight = 64;
    
    s64 FrameStartCycles = VisibleFrame->FrameStartCycles;
    s64 FrameTotalCycles = VisibleFrame->FrameEndCycles - VisibleFrame->FrameStartCycles;
    
    debug_scope_record_list* ThreadScopeCalls = GetScopeListForThreadInFrame(GlobalDebugServices,
                                                                             VisibleFrame);
    
    scope_record* HotRecord = 0;
    scope_name* HotRecordName = 0;
    
    char Backbuffer[256];
    gs_string String = MakeString(Backbuffer, 0, 256);
    for (s32 i = 0; i < ThreadScopeCalls->Count; i++)
    {
        scope_record* Record = ThreadScopeCalls->Calls + i;
        scope_name* Name = GetOrAddNameHashEntry(VisibleFrame, Record->NameHash);
        r32 PercentStart = (r32)(Record->StartCycles - FrameStartCycles) / (r32)FrameTotalCycles;
        r32 PercentEnd = (r32)(Record->EndCycles - FrameStartCycles) / (r32)FrameTotalCycles;
        
        r32 PixelStart = Bounds.Min.x + (Width * PercentStart);
        r32 PixelEnd = Bounds.Min.x + (Width * PercentEnd);
        r32 MinY = Bounds.Max.y - ((Record->CallDepth + 1) * DepthHeight);
        rect2 ScopeBounds = {
            v2{ PixelStart, MinY },
            v2{ PixelEnd, MinY + (DepthHeight - 4) }
        };
        if (Rect2Width(ScopeBounds) >= 1)
        {
            v4 Color = ThreadColors[0];
            if (PointIsInRect(ScopeBounds, Interface->Mouse.Pos))
            {
                Color = GreenV4;
                HotRecord = Record;
                HotRecordName = Name;
            }
            
            ui_FillRect(Interface, ScopeBounds, Color);
            ui_OutlineRect(Interface, ScopeBounds, 1, BlackV4);
        }
    }
    
    if (HotRecord != 0)
    {
        PrintF(&String, "%S : %d - %d", HotRecordName->Name, HotRecord->StartCycles, HotRecord->EndCycles);
        
        rect2 TextBounds = MakeRect2MinDim(Interface->Mouse.Pos, v2{256, 32});
        ui_Label(Interface, String, TextBounds);
    }
}

internal void
RenderProfiler_ListVisualization(ui_interface* Interface, ui_widget* Layout, debug_frame* VisibleFrame, gs_memory_arena* Memory)
{
    char Backbuffer[256];
    gs_string String = MakeString(Backbuffer, 0, 256);
    
    ui_column_spec ColumnWidths[] = {
        { UIColumnSize_Fixed, 256 },
        { UIColumnSize_Fixed, 128 },
        { UIColumnSize_Fixed, 128 },
        { UIColumnSize_Fixed, 128 },
        { UIColumnSize_Fixed, 128 }};
    ui_BeginRow(Interface, 5, &ColumnWidths[0]);
    {
        ui_Label(Interface, MakeString("Procedure"));
        ui_Label(Interface, MakeString("% Frame"));
        ui_Label(Interface, MakeString("Seconds"));
        ui_Label(Interface, MakeString("Cycles"));
        ui_Label(Interface, MakeString("Calls"));
    }
    ui_EndRow(Interface);
    
    for (s32 n = 0; n < VisibleFrame->ScopeNamesMax; n++)
    {
        scope_name NameEntry = VisibleFrame->ScopeNamesHash[n];
        if (NameEntry.Hash != 0)
        {
            collated_scope_record* CollatedRecord = VisibleFrame->CollatedScopes + n;
            
            ui_BeginRow(Interface, 5, &ColumnWidths[0]);
            {
                PrintF(&String, "%S", NameEntry.Name);
                ui_Label(Interface, String);
                
                PrintF(&String, "%f%%", CollatedRecord->PercentFrameTime);
                ui_Label(Interface, String);
                
                PrintF(&String, "%fs", CollatedRecord->TotalSeconds);
                ui_Label(Interface, String);
                
                PrintF(&String, "%dcy", CollatedRecord->TotalCycles);
                ui_Label(Interface, String);
                
                PrintF(&String, "%d", CollatedRecord->CallCount);
                ui_Label(Interface, String);
            }
            ui_EndRow(Interface);
        }
    }
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_profiler);
internal void
ProfilerView_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    gs_memory_arena* Memory = State->Transient;
    gs_string String = PushString(Memory, 256);
    
    v4 FrameColors[] = { GreenV4, YellowV4, RedV4, WhiteV4 };
    
    r32 FrameListHeight = 64;
    rect2 FrameListBounds, ProcListBounds;
    RectHSplitAtDistanceFromTop(PanelBounds, FrameListHeight, &FrameListBounds, &ProcListBounds);
    rect2 FrameListInner = RectInset(FrameListBounds, 4);
    
    r32 SingleFrameStep = Rect2Width(FrameListInner) / DEBUG_FRAME_COUNT;
    r32 SingleFrameWidth = (r32)((s32)SingleFrameStep - 2);
    
    ui_OutlineRect(&State->Interface, FrameListBounds, 2, WhiteV4);
    if (MouseButtonHeldDown(Context.Mouse.LeftButtonState))
    {
        if (PointIsInRect(FrameListBounds, Context.Mouse.Pos))
        {
            v2 LocalMouse = Rect2GetRectLocalPoint(FrameListBounds, Context.Mouse.Pos);
            s32 ClosestFrameIndex = (LocalMouse.x / SingleFrameStep);
            if (ClosestFrameIndex >= 0 && ClosestFrameIndex < DEBUG_FRAME_COUNT)
            {
                GlobalDebugServices->RecordFrames = false;
                GlobalDebugServices->CurrentDebugFrame = ClosestFrameIndex;
            }
        }
    }
    
    rect2 FrameBounds = MakeRect2MinDim(FrameListInner.Min, v2{SingleFrameWidth, Rect2Height(FrameListInner)});
    for (s32 F = 0; F < DEBUG_FRAME_COUNT; F++)
    {
        rect2 PositionedFrameBounds = Rect2TranslateX(FrameBounds, F * SingleFrameStep);
        s32 FramesAgo = (GlobalDebugServices->CurrentDebugFrame - F);
        if (FramesAgo < 0) { FramesAgo += DEBUG_FRAME_COUNT; }
        v4 Color = FrameColors[Clamp(0, FramesAgo, 3)];
        ui_FillRect(&State->Interface, PositionedFrameBounds, Color);
    }
    
    debug_frame* VisibleFrame = GetLastDebugFrame(GlobalDebugServices);
    
    ui_widget* Layout = ui_PushLayout(&State->Interface, ProcListBounds, LayoutDirection_TopDown, MakeString("Profiler Layout"));
    
    ui_BeginRow(&State->Interface, 4);
    {
        s64 FrameStartCycles = VisibleFrame->FrameStartCycles;
        s64 FrameTotalCycles = VisibleFrame->FrameEndCycles - VisibleFrame->FrameStartCycles;
        u32 CurrentDebugFrame = GlobalDebugServices->CurrentDebugFrame - 1;
        PrintF(&String, "Frame %d", CurrentDebugFrame);
        ui_Label(&State->Interface, String);
        
        PrintF(&String, "Total Cycles: %lld", FrameTotalCycles);
        ui_Label(&State->Interface, String);
        
        // NOTE(NAME): Skipping a space for aesthetic reasons, not functional, and could
        // be removed, or used for something else
        ui_ReserveBounds(&State->Interface, Layout, true);
        
        if (ui_Button(&State->Interface, MakeString("Resume Recording")))
        {
            GlobalDebugServices->RecordFrames = true;
        }
    }
    ui_EndRow(&State->Interface);
    
    ui_BeginRow(&State->Interface, 8);
    {
        if (ui_Button(&State->Interface, MakeString("Scope View")))
        {
            GlobalDebugServices->Interface.FrameView = FRAME_VIEW_PROFILER;
        }
        if (ui_Button(&State->Interface, MakeString("List View")))
        {
            GlobalDebugServices->Interface.FrameView = FRAME_VIEW_SCOPE_LIST;
        }
    }
    ui_EndRow(&State->Interface);
    
    if (GlobalDebugServices->Interface.FrameView == FRAME_VIEW_PROFILER)
    {
        RenderProfiler_ScopeVisualization(&State->Interface, Layout, VisibleFrame, Memory);
    }
    else
    {
        RenderProfiler_ListVisualization(&State->Interface, Layout, VisibleFrame, Memory);
    }
    
    ui_PopLayout(&State->Interface);
}


#define FOLDHAUS_PANEL_PROFILER_H
#endif // FOLDHAUS_PANEL_PROFILER_H
