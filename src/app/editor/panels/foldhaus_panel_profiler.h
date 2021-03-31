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
RenderProfiler_ScopeVisualization(ui_interface* Interface, ui_widget* Layout, debug_frame* VisibleFrame, gs_memory_arena* Transient)
{
    rect2 Bounds = ui_LayoutRemaining(*Layout);
    r32 Width = Rect2Width(Bounds);
    r32 DepthHeight = 32;
    
    s64 FrameStartCycles = VisibleFrame->FrameStartCycles;
    r32 FrameTotalCycles = (r32)(VisibleFrame->FrameEndCycles - VisibleFrame->FrameStartCycles);
    
    r32 NextThreadTop = Bounds.Max.y;
    
    for (s32 t = 0; t < VisibleFrame->ThreadCount; t++)
    {
        debug_scope_record_list ThreadCalls = VisibleFrame->ThreadCalls[t];
        
        gs_string String = PushString(Transient, 256);
        
        r32 ThreadScopeMin = Bounds.Max.y;
        
        //PrintF(&String, "Thread %d", ThreadCalls.ThreadId);
        //ui_Label(Interface, String, rect2{ThreadScopeMin);
        
        r32 Hue = (r32)(t) / (r32)(VisibleFrame->ThreadCount);
        Hue += (.5f * (t % 2));
        v4 ThreadHSV = v4{ 360.0f * Hue, .5f, 1.0f, 1.0f };
        v4 ThreadRGB = HSVToRGB(ThreadHSV);
        
        for (s32 i = 0; i < ThreadCalls.Count; i++)
        {
            scope_record* Record = ThreadCalls.Calls + i;
            scope_name* Name = GetOrAddNameHashEntry(VisibleFrame, Record->NameHash);
            s64 OffsetStart = Record->StartCycles - FrameStartCycles;
            s64 OffsetEnd = Record->EndCycles - FrameStartCycles;
            r32 PercentStart = (r32)(OffsetStart) / FrameTotalCycles;
            r32 PercentEnd = (r32)(OffsetEnd) / FrameTotalCycles;
            r32 PercentWidth = PercentEnd - PercentStart;
            
            rect2 ScopeBounds = {
                v2{0, 0},
                v2{PercentWidth * Width, DepthHeight - 4},
            };
            v2 Offset = {
                Bounds.Min.x + (PercentStart * Width),
                NextThreadTop - ((Record->CallDepth + 1) * DepthHeight)
            };
            ScopeBounds = Rect2Translate(ScopeBounds, Offset);
            ThreadScopeMin = Min(ScopeBounds.Min.y, ThreadScopeMin);
            
            if (Rect2Width(ScopeBounds) >= 1)
            {
                v4 Color = ThreadRGB;
                if (PointIsInRect(ScopeBounds, Interface->Mouse.Pos))
                {
                    Color = GreenV4;
                    
                    ui_BeginMousePopup(Interface, rect2{ 25, 25, 300, 57 }, LayoutDirection_TopDown, MakeString("Hover"));
                    {
                        s64 Cycles = (Record->EndCycles - Record->StartCycles);
                        r32 PercentFrame = (r32)(Cycles) / FrameTotalCycles;
                        PrintF(&String, "%S : %.2f%% frame | %dcy",
                               Name->Name,
                               PercentFrame,
                               Cycles);
                        ui_Label(Interface, String);
                    }
                    ui_EndMousePopup(Interface);
                }
                
                ui_FillRect(Interface, ScopeBounds, Color);
                ui_OutlineRect(Interface, ScopeBounds, 1, BlackV4);
            }
        }
        
        NextThreadTop = ThreadScopeMin;
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
    
    s32 CountedScopes = 0;
    for (s32 n = 0; n < VisibleFrame->ScopeNamesMax; n++)
    {
        scope_name NameEntry = VisibleFrame->ScopeNamesHash[n];
        if (NameEntry.Hash != 0)
        {
            CountedScopes += 1;
        }
    }
    
    ui_BeginList(Interface, MakeString("Scope List"), 10, CountedScopes);
    ui_BeginRow(Interface, 5, &ColumnWidths[0]);
    for (s32 n = 0; n < VisibleFrame->ScopeNamesMax; n++)
    {
        scope_name NameEntry = VisibleFrame->ScopeNamesHash[n];
        if (NameEntry.Hash != 0)
        {
            collated_scope_record* CollatedRecord = VisibleFrame->CollatedScopes + n;
            
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
    }
    ui_EndRow(Interface);
    ui_EndList(Interface);
}

struct mem_amt
{
    u64 OrigSize;
    r64 Size;
    char* Units;
};

internal mem_amt
GetMemAmt (u64 BytesCount)
{
    mem_amt Result = {};
    Result.OrigSize = BytesCount;
    Result.Size = (r64)BytesCount;
    Result.Units = "bytes";
    
    u32 i = 0;
    char* UnitList[] = { "kb", "mb", "gb", "tb" };
    while (Result.Size > 1024) {
        Result.Size /= 1024.0;
        Result.Units = UnitList[i++];
    }
    
    return Result;
}

internal void
RenderProfiler_MemoryView(ui_interface* Interface, ui_widget* Layout, app_state* State, context Context, gs_memory_arena* Memory)
{
    gs_allocator_debug Debug = *Context.ThreadContext.Allocator.Debug;
    gs_string TempString = PushString(State->Transient, 256);
    
    mem_amt MemFootprint = GetMemAmt(Debug.TotalAllocSize);
    u64 AllocCount = Debug.AllocationsCount;
    
    
    PrintF(&TempString, "Total Memory Size: %.2f %s | Allocations: %lld", MemFootprint.Size, MemFootprint.Units, AllocCount);
    ui_Label(Interface, TempString);
    
    ui_column_spec ColumnWidths[] = {
        { UIColumnSize_Fill, 0 },
        { UIColumnSize_Fixed,256 },
    };
    ui_BeginRow(Interface, 2, &ColumnWidths[0]);
    {
        ui_Label(Interface, MakeString("Location"));
        ui_Label(Interface, MakeString("Alloc Size"));
    }
    ui_EndRow(Interface);
    
    ui_BeginList(Interface, MakeString("Alloc List"), 10, Debug.AllocationsCount);
    ui_BeginRow(Interface, 2, &ColumnWidths[0]);
    for (s32 n = 0; n < Debug.AllocationsCount; n++)
    {
        gs_debug_allocation A = Debug.Allocations[n];
        
        PrintF(&TempString, "%S", A.Location);
        ui_Label(Interface, TempString);
        
        mem_amt Amt = GetMemAmt(A.Size);
        
        PrintF(&TempString, "%.2f %s", Amt.Size, Amt.Units);
        ui_Label(Interface, TempString);
    }
    ui_EndRow(Interface);
    ui_EndList(Interface);
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
    
    s32 FramesToDisplay = DEBUG_FRAME_COUNT;
    if (FramesToDisplay != 0)
    {
        r32 SingleFrameStep = Rect2Width(FrameListInner) / FramesToDisplay;
        r32 SingleFrameWidth = (r32)((s32)SingleFrameStep - 2);
        
        ui_OutlineRect(&State->Interface, FrameListBounds, 2, WhiteV4);
        if (MouseButtonHeldDown(Context.Mouse.LeftButtonState))
        {
            if (PointIsInRect(FrameListBounds, Context.Mouse.Pos))
            {
                v2 LocalMouse = Rect2GetRectLocalPoint(FrameListBounds, Context.Mouse.Pos);
                s32 ClosestFrameIndex = (LocalMouse.x / SingleFrameStep);
                if (ClosestFrameIndex >= 0 && ClosestFrameIndex < FramesToDisplay)
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
    }
    
    ui_widget* Layout = ui_PushLayout(&State->Interface, ProcListBounds, LayoutDirection_TopDown, MakeString("Profiler Layout"));
    
    debug_frame* VisibleFrame = GetLastDebugFrame(GlobalDebugServices);
    if (VisibleFrame)
    {
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
    }
    
    ui_BeginRow(&State->Interface, 8);
    {
        if (ui_Button(&State->Interface, MakeString("Profiler")))
        {
            GlobalDebugServices->Interface.FrameView = DebugUI_Profiler;
        }
        if (ui_Button(&State->Interface, MakeString("List View")))
        {
            GlobalDebugServices->Interface.FrameView = DebugUI_ScopeList;
        }
        if (ui_Button(&State->Interface, MakeString("Memory")))
        {
            GlobalDebugServices->Interface.FrameView = DebugUI_MemoryView;
        }
    }
    ui_EndRow(&State->Interface);
    
    switch (GlobalDebugServices->Interface.FrameView)
    {
        case DebugUI_Profiler:
        {
            if (VisibleFrame) 
            {
                RenderProfiler_ScopeVisualization(&State->Interface, Layout, VisibleFrame, Memory);
            }
        }break;
        
        case DebugUI_ScopeList:
        {
            if (VisibleFrame)
            {
                RenderProfiler_ListVisualization(&State->Interface, Layout, VisibleFrame, Memory);
            }
        }break;
        
        case DebugUI_MemoryView:
        {
            RenderProfiler_MemoryView(&State->Interface, Layout, State, Context, Memory);
        }break;
        
        InvalidDefaultCase;
    }
    
    ui_PopLayout(&State->Interface, MakeString("Profiler Layout"));
}


#define FOLDHAUS_PANEL_PROFILER_H
#endif // FOLDHAUS_PANEL_PROFILER_H
