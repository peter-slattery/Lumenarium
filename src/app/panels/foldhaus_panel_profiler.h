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
ProfilerView_Init(panel* Panel, app_state* State)
{
    
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_profiler);
internal void
ProfilerView_Cleanup(panel* Panel, app_state* State)
{
    
}

internal void
RenderProfiler_ScopeVisualization(ui_interface* Interface, ui_layout Layout, debug_frame* VisibleFrame, memory_arena* Memory)
{
    v4 ThreadColors[] = {
        v4{.73f, .33f, .83f, 1},
        v4{0,   .50f, .50f, 1},
        v4{.83f, 0,    0, 1},
        v4{.33f, .49f, .83f, 1},
        v4{.74f, .40f, .25f, 1},
    };
    
    rect Bounds = ui_LayoutRemaining(Layout);
    r32 Width = gs_Width(Bounds);
    r32 DepthHeight = 64;
    
    s64 FrameStartCycles = VisibleFrame->FrameStartCycles;
    s64 FrameTotalCycles = VisibleFrame->FrameEndCycles - VisibleFrame->FrameStartCycles;
    
    debug_scope_record_list* ThreadScopeCalls = GetScopeListForThreadInFrame(GlobalDebugServices,
                                                                             VisibleFrame);
    
    scope_record* HotRecord = 0;
    scope_name* HotRecordName = 0;
    
    MakeStringBuffer(String, 256);
    for (s32 i = 0; i < ThreadScopeCalls->Count; i++)
    {
        scope_record* Record = ThreadScopeCalls->Calls + i;
        scope_name* Name = GetOrAddNameHashEntry(VisibleFrame, Record->NameHash);
        r32 PercentStart = (r32)(Record->StartCycles - FrameStartCycles) / (r32)FrameTotalCycles;
        r32 PercentEnd = (r32)(Record->EndCycles - FrameStartCycles) / (r32)FrameTotalCycles;
        
        r32 PixelStart = Bounds.Min.x + (Width * PercentStart);
        r32 PixelEnd = Bounds.Min.x + (Width * PercentEnd);
        r32 MinY = Bounds.Max.y - ((Record->CallDepth + 1) * DepthHeight);
        rect ScopeBounds = {
            v2{ PixelStart, MinY },
            v2{ PixelEnd, MinY + (DepthHeight - 4) }
        };
        if (gs_Width(ScopeBounds) >= 1)
        {
            v4 Color = ThreadColors[0];
            if (gs_PointIsInRect(Interface->Mouse.Pos, ScopeBounds))
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
        ui_TextBox(Interface, gs_MakeRectMinWidth(Interface->Mouse.Pos, v2{256, 32}), String, BlackV4, WhiteV4);
    }
}

internal void
RenderProfiler_ListVisualization(ui_interface* Interface, ui_layout Layout, debug_frame* VisibleFrame, memory_arena* Memory)
{
    MakeStringBuffer(String, 256);
    
    r32 ColumnWidths[] = {256, 128, 128, 128, 128};
    ui_StartRow(&Layout, 5, &ColumnWidths[0]);
    {
        ui_LayoutDrawString(Interface, &Layout, MakeStringLiteral("Procedure"), Interface->Style.TextColor);
        ui_LayoutDrawString(Interface, &Layout, MakeStringLiteral("% Frame"), Interface->Style.TextColor);
        ui_LayoutDrawString(Interface, &Layout, MakeStringLiteral("Seconds"), Interface->Style.TextColor);
        ui_LayoutDrawString(Interface, &Layout, MakeStringLiteral("Cycles"), Interface->Style.TextColor);
        ui_LayoutDrawString(Interface, &Layout, MakeStringLiteral("Calls"), Interface->Style.TextColor);
    }
    ui_EndRow(&Layout);
    
    for (s32 n = 0; n < VisibleFrame->ScopeNamesMax; n++)
    {
        scope_name NameEntry = VisibleFrame->ScopeNamesHash[n];
        if (NameEntry.Hash != 0)
        {
            collated_scope_record* CollatedRecord = VisibleFrame->CollatedScopes + n;
            
            ui_StartRow(&Layout, 5, &ColumnWidths[0]);
            {
                PrintF(&String, "%S", NameEntry.Name);
                ui_LayoutDrawString(Interface, &Layout, String, Interface->Style.TextColor);
                
                PrintF(&String, "%f%%", CollatedRecord->PercentFrameTime);
                ui_LayoutDrawString(Interface, &Layout, String, Interface->Style.TextColor);
                
                PrintF(&String, "%fs", CollatedRecord->TotalSeconds);
                ui_LayoutDrawString(Interface, &Layout, String, Interface->Style.TextColor);
                
                PrintF(&String, "%dcy", CollatedRecord->TotalCycles);
                ui_LayoutDrawString(Interface, &Layout, String, Interface->Style.TextColor);
                
                PrintF(&String, "%d", CollatedRecord->CallCount);
                ui_LayoutDrawString(Interface, &Layout, String, Interface->Style.TextColor);
            }
            ui_EndRow(&Layout);
        }
    }
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_profiler);
internal void
ProfilerView_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    memory_arena* Memory = &State->Transient;
    string String = InitializeEmptyString(PushArray(Memory, char, 256), 256);
    
    v4 FrameColors[] = { GreenV4, YellowV4, RedV4, WhiteV4 };
    
    r32 FrameListHeight = 64;
    rect FrameListBounds, ProcListBounds;
    gs_HSplitRectAtDistanceFromTop(PanelBounds, FrameListHeight, &FrameListBounds, &ProcListBounds);
    rect FrameListInner = gs_InsetRect(FrameListBounds, 4);
    
    r32 SingleFrameStep = gs_Width(FrameListInner) / DEBUG_FRAME_COUNT;
    r32 SingleFrameWidth = (r32)((s32)SingleFrameStep - 2);
    
    ui_OutlineRect(&State->Interface, FrameListBounds, 2, WhiteV4);
    if (gs_PointIsInRect(Context.Mouse.Pos, FrameListBounds) && MouseButtonHeldDown(Context.Mouse.LeftButtonState))
    {
        v2 LocalMouse = gs_TransformPointIntoRectSpace(Context.Mouse.Pos, FrameListBounds);
        s32 ClosestFrameIndex = (LocalMouse.x / SingleFrameStep);
        if (ClosestFrameIndex >= 0 && ClosestFrameIndex < DEBUG_FRAME_COUNT)
        {
            GlobalDebugServices->RecordFrames = false;
            GlobalDebugServices->CurrentDebugFrame = ClosestFrameIndex;
        }
    }
    
    rect FrameBounds = gs_MakeRectMinWidth(FrameListInner.Min, v2{SingleFrameWidth, gs_Height(FrameListInner)});
    for (s32 F = 0; F < DEBUG_FRAME_COUNT; F++)
    {
        rect PositionedFrameBounds = gs_TranslateRectX(FrameBounds, F * SingleFrameStep);
        s32 FramesAgo = (GlobalDebugServices->CurrentDebugFrame - F);
        if (FramesAgo < 0) { FramesAgo += DEBUG_FRAME_COUNT; }
        v4 Color = FrameColors[GSClamp(0, FramesAgo, 3)];
        ui_FillRect(&State->Interface, PositionedFrameBounds, Color);
    }
    
    debug_frame* VisibleFrame = GetLastDebugFrame(GlobalDebugServices);
    
    ui_layout Layout = ui_CreateLayout(State->Interface, ProcListBounds);
    ui_StartRow(&Layout, 4);
    {
        s64 FrameStartCycles = VisibleFrame->FrameStartCycles;
        s64 FrameTotalCycles = VisibleFrame->FrameEndCycles - VisibleFrame->FrameStartCycles;
        u32 CurrentDebugFrame = GlobalDebugServices->CurrentDebugFrame - 1;
        PrintF(&String, "Frame %d", CurrentDebugFrame);
        ui_LayoutDrawString(&State->Interface, &Layout, String, WhiteV4);
        
        PrintF(&String, "Total Cycles: %lld", FrameTotalCycles);
        ui_LayoutDrawString(&State->Interface, &Layout, String, WhiteV4);
        
        // NOTE(NAME): Skipping a space for aesthetic reasons, not functional, and could
        // be removed, or used for something else
        ui_ReserveElementBounds(&Layout);
        
        if (ui_LayoutButton(&State->Interface, &Layout, MakeString("Resume Recording")))
        {
            GlobalDebugServices->RecordFrames = true;
        }
    }
    ui_EndRow(&Layout);
    
    ui_StartRow(&Layout, 8);
    {
        if (ui_LayoutButton(&State->Interface, &Layout, MakeString("Scope View")))
        {
            GlobalDebugServices->Interface.FrameView = FRAME_VIEW_PROFILER;
        }
        if (ui_LayoutButton(&State->Interface, &Layout, MakeString("List View")))
        {
            GlobalDebugServices->Interface.FrameView = FRAME_VIEW_SCOPE_LIST;
        }
    }
    ui_EndRow(&Layout);
    
    if (GlobalDebugServices->Interface.FrameView == FRAME_VIEW_PROFILER)
    {
        RenderProfiler_ScopeVisualization(&State->Interface, Layout, VisibleFrame, Memory);
    }
    else
    {
        RenderProfiler_ListVisualization(&State->Interface, Layout, VisibleFrame, Memory);
    }
}


#define FOLDHAUS_PANEL_PROFILER_H
#endif // FOLDHAUS_PANEL_PROFILER_H
