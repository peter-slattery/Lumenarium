//
// File: foldhaus_panel_animation_timeline.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_ANIMATION_TIMELINE_H

// Colors
global_variable v4 TimeSliderColor = v4{.36f, .52f, .78f, 1.f};

//
struct animation_timeline_state
{
    s32 VisibleFrameRangeMin;
    s32 VisibleFrameRangeMax;
};

// TODO(Peter): Move this over to the animation system just as frame_range
struct timeline_frame_range
{
    u32 FrameMin;
    u32 FrameMax;
};

inline b32
FrameIsInRange(u32 Frame, timeline_frame_range Range)
{
    b32 Result = (Frame >= Range.FrameMin) && (Frame <= Range.FrameMax);
    return Result;
}

internal u32
GetFrameCount(timeline_frame_range Range)
{
    u32 Result = Range.FrameMax - Range.FrameMin;
    return Result;
}

internal r32
FrameToPercentRange(s32 Frame, timeline_frame_range Range)
{
    r32 Result = (r32)(Frame - Range.FrameMin);
    Result = Result / GetFrameCount(Range);
    return Result;
}

internal u32
PercentToFrameInRange(r32 Percent, timeline_frame_range Range)
{
    u32 Result = Range.FrameMin + (u32)(Percent * GetFrameCount(Range));
    return Result;
}

internal u32 
ClampFrameToRange(u32 Frame, timeline_frame_range Range)
{
    u32 Result = Frame;
    if (Result < Range.FrameMin)
    {
        Result = Range.FrameMin;
    }
    else if (Result > Range.FrameMax)
    {
        Result = Range.FrameMax;
    }
    return Result;
}

//
//

inline u32
GetFrameFromPointInAnimationPanel(v2 Point, rect PanelBounds, u32 StartFrame, u32 EndFrame)
{
    r32 HorizontalPercentOfBounds = (Point.x - PanelBounds.Min.x) / (PanelBounds.Max.x - PanelBounds.Min.x);
    u32 TimeAtPoint = (u32)(HorizontalPercentOfBounds * (EndFrame - StartFrame)) + StartFrame;
    return TimeAtPoint;
}

inline s32
GetXPositionFromFrameInAnimationPanel (u32 Frame, rect PanelBounds, s32 StartFrame, s32 EndFrame)
{
    r32 PercentOfTimeline = (r32)(Frame - StartFrame) / (r32)(EndFrame - StartFrame);
    s32 XPositionAtFrame = (PercentOfTimeline * Width(PanelBounds)) + PanelBounds.Min.x;
    return XPositionAtFrame;
}

internal gs_list_handle
AddAnimationBlock(u32 StartFrame, u32 EndFrame, u32 AnimationProcHandle, animation_system* AnimationSystem)
{
    gs_list_handle Result = {0};
    animation_block NewBlock = {0};
    NewBlock.StartFrame = StartFrame;
    NewBlock.EndFrame = EndFrame;
    NewBlock.AnimationProcHandle = AnimationProcHandle;
    Result = AnimationSystem->Blocks.PushElementOnList(NewBlock);
    return Result;
}

internal gs_list_handle
AddAnimationBlockAtCurrentTime (u32 AnimationProcHandle, animation_system* System)
{
    u32 NewBlockStart = System->CurrentFrame;
    u32 NewBlockEnd = NewBlockStart + SecondsToFrames(3, *System);
    gs_list_handle Result = AddAnimationBlock(NewBlockStart, NewBlockEnd, AnimationProcHandle, System);
    return Result;
}

internal void
DeleteAnimationBlock(gs_list_handle AnimationBlockHandle, app_state* State)
{
    State->AnimationSystem.Blocks.FreeElementWithHandle(AnimationBlockHandle);
    State->SelectedAnimationBlockHandle = {0};
}

internal void
SelectAnimationBlock(gs_list_handle BlockHandle, app_state* State)
{
    State->SelectedAnimationBlockHandle = BlockHandle;
}

internal void
DeselectCurrentAnimationBlock(app_state* State)
{
    State->SelectedAnimationBlockHandle = {};
}

FOLDHAUS_INPUT_COMMAND_PROC(DeleteAnimationBlockCommand)
{
    if(ListHandleIsValid(State->SelectedAnimationBlockHandle))
    {
        DeleteAnimationBlock(State->SelectedAnimationBlockHandle, State);
    }
}

//
// Drag Time Marker
//

OPERATION_STATE_DEF(drag_time_marker_operation_state)
{
    rect TimelineBounds;
    s32 StartFrame;
    s32 EndFrame;
};

OPERATION_RENDER_PROC(UpdateDragTimeMarker)
{
    drag_time_marker_operation_state* OpState = (drag_time_marker_operation_state*)Operation.OpStateMemory;
    u32 FrameAtMouseX = GetFrameFromPointInAnimationPanel(Mouse.Pos, OpState->TimelineBounds, OpState->StartFrame, OpState->EndFrame);
    State->AnimationSystem.CurrentFrame = FrameAtMouseX;
}

FOLDHAUS_INPUT_COMMAND_PROC(EndDragTimeMarker)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command DragTimeMarkerCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndDragTimeMarker },
};

internal void
StartDragTimeMarker(rect TimelineBounds, timeline_frame_range VisibleFrames, app_state* State)
{
    operation_mode* DragTimeMarkerMode = ActivateOperationModeWithCommands(&State->Modes, DragTimeMarkerCommands, UpdateDragTimeMarker);
    
    drag_time_marker_operation_state* OpState = CreateOperationState(DragTimeMarkerMode,
                                                                     &State->Modes,
                                                                     drag_time_marker_operation_state);
    OpState->StartFrame = VisibleFrames.FrameMin;
    OpState->EndFrame = VisibleFrames.FrameMax;
    OpState->TimelineBounds = TimelineBounds;
}

// --------------------

//
// Drag Animation Clip
//

#define CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE 10

OPERATION_STATE_DEF(drag_animation_clip_state)
{
    rect TimelineBounds;
    s32 AnimationPanel_StartFrame;
    s32 AnimationPanel_EndFrame;
    s32 SelectedClip_InitialStartFrame;
    s32 SelectedClip_InitialEndFrame;
};

internal u32
AttemptToSnapPosition(u32 SnappingFrame, u32 SnapToFrame)
{
    u32 Result = SnappingFrame;
    s32 SnapDistance = 5;
    if (GSAbs((s32)SnappingFrame - (s32)SnapToFrame) <= SnapDistance)
    {
        Result = SnapToFrame;
    }
    return Result;
}

OPERATION_RENDER_PROC(UpdateDragAnimationClip)
{
    drag_animation_clip_state* OpState = (drag_animation_clip_state*)Operation.OpStateMemory;
    
    u32 ClipInitialStartFrameXPosition = GetXPositionFromFrameInAnimationPanel(OpState->SelectedClip_InitialStartFrame, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame);
    u32 ClipInitialEndFrameXPosition = GetXPositionFromFrameInAnimationPanel(OpState->SelectedClip_InitialEndFrame, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame);
    
    u32 FrameAtMouseDownX = GetFrameFromPointInAnimationPanel(Mouse.DownPos, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame);
    
    u32 FrameAtMouseX = GetFrameFromPointInAnimationPanel(Mouse.Pos, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame);
    s32 FrameOffset = (s32)FrameAtMouseX - (s32)FrameAtMouseDownX;
    
    animation_block* AnimationBlock = State->AnimationSystem.Blocks.GetElementWithHandle(State->SelectedAnimationBlockHandle);
    if (!AnimationBlock)
    {
        EndCurrentOperationMode(State, {}, Mouse);
        return;
    }
    
    if (GSAbs(Mouse.DownPos.x - ClipInitialStartFrameXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        u32 NewStartFrame = OpState->SelectedClip_InitialStartFrame + FrameOffset;
        if (FrameOffset < 0)
        {
            for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
            {
                gs_list_entry<animation_block>* OtherBlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
                if (OtherBlockEntry->Free.NextFreeEntry != 0) { continue; }
                animation_block OtherBlock = OtherBlockEntry->Value;
                NewStartFrame = AttemptToSnapPosition(NewStartFrame, OtherBlock.EndFrame);
            }
        }
        else
        {
            if (NewStartFrame >= AnimationBlock->EndFrame)
            {
                NewStartFrame = AnimationBlock->EndFrame - 1;
            }
        }
        AnimationBlock->StartFrame = NewStartFrame; 
    }
    else if (GSAbs(Mouse.DownPos.x - ClipInitialEndFrameXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        r32 NewEndFrame = OpState->SelectedClip_InitialEndFrame + FrameOffset;
        if (FrameOffset > 0)
        {
            for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
            {
                gs_list_entry<animation_block>* OtherBlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
                if (OtherBlockEntry->Free.NextFreeEntry != 0) { continue; }
                animation_block OtherBlock = OtherBlockEntry->Value;
                NewEndFrame = AttemptToSnapPosition(NewEndFrame, OtherBlock.StartFrame);
            }
        }
        else
        {
            if(NewEndFrame <= AnimationBlock->StartFrame)
            {
                NewEndFrame = AnimationBlock->StartFrame + 1;
            }
        }
        AnimationBlock->EndFrame = NewEndFrame;
    }
    else
    {
        u32 NewStartFrame = OpState->SelectedClip_InitialStartFrame + FrameOffset;
        u32 NewEndFrame = OpState->SelectedClip_InitialEndFrame + FrameOffset;
        for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
        {
            gs_list_entry<animation_block>* OtherBlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
            if (OtherBlockEntry->Free.NextFreeEntry != 0) { continue; }
            animation_block OtherBlock = OtherBlockEntry->Value;
            
            u32 SnapFramesAmount = 0;
            if (FrameOffset > 0)
            {
                u32 FinalEndFrame = AttemptToSnapPosition(NewEndFrame, OtherBlock.StartFrame);
                SnapFramesAmount = FinalEndFrame - NewEndFrame;
            }
            else if (FrameOffset < 0)
            {
                u32 FinalStartFrame = AttemptToSnapPosition(NewStartFrame, OtherBlock.EndFrame);
                SnapFramesAmount = FinalStartFrame - NewStartFrame;
            }
            NewEndFrame += SnapFramesAmount;
            NewStartFrame += SnapFramesAmount;
        }
        AnimationBlock->StartFrame = NewStartFrame;
        AnimationBlock->EndFrame = NewEndFrame;
    }
    
    s32 VisibleStartFrame = OpState->AnimationPanel_StartFrame;
    s32 VisibleEndFrame = OpState->AnimationPanel_EndFrame;
    AnimationBlock->StartFrame = (u32)GSClamp(VisibleStartFrame, (s32)AnimationBlock->StartFrame, VisibleEndFrame);
    AnimationBlock->EndFrame = (u32)GSClamp(VisibleStartFrame, (s32)AnimationBlock->EndFrame, VisibleEndFrame);
}

input_command DragAnimationClipCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndCurrentOperationMode },
};

internal void
SelectAndBeginDragAnimationBlock(gs_list_handle BlockHandle, timeline_frame_range VisibleRange, rect TimelineBounds, app_state* State)
{
    SelectAnimationBlock(BlockHandle, State);
    
    operation_mode* DragAnimationClipMode = ActivateOperationModeWithCommands(&State->Modes, DragAnimationClipCommands, UpdateDragAnimationClip);
    
    drag_animation_clip_state* OpState = CreateOperationState(DragAnimationClipMode,
                                                              &State->Modes,
                                                              drag_animation_clip_state);
    OpState->TimelineBounds = TimelineBounds;
    OpState->AnimationPanel_StartFrame = VisibleRange.FrameMin;
    OpState->AnimationPanel_EndFrame = VisibleRange.FrameMax;
    
    animation_block* SelectedBlock = State->AnimationSystem.Blocks.GetElementWithHandle(BlockHandle);
    OpState->SelectedClip_InitialStartFrame = SelectedBlock->StartFrame;
    OpState->SelectedClip_InitialEndFrame = SelectedBlock->EndFrame;
}
// -------------------

FOLDHAUS_INPUT_COMMAND_PROC(AddAnimationBlockCommand)
{
    panel_and_bounds ActivePanel = GetPanelContainingPoint(Mouse.Pos, &State->PanelSystem, State->WindowBounds);
    u32 MouseDownFrame = GetFrameFromPointInAnimationPanel(Mouse.Pos, ActivePanel.Bounds, State->AnimationSystem.StartFrame, State->AnimationSystem.EndFrame);
    gs_list_handle NewBlockHandle = AddAnimationBlock(MouseDownFrame, MouseDownFrame + SecondsToFrames(3, State->AnimationSystem), 4, &State->AnimationSystem);
    SelectAnimationBlock(NewBlockHandle, State);
}

input_command AnimationTimeline_Commands[] = {
    { KeyCode_X, KeyCode_Invalid, Command_Began, DeleteAnimationBlockCommand },
    { KeyCode_A, KeyCode_Invalid, Command_Began, AddAnimationBlockCommand },
};
s32 AnimationTimeline_CommandsCount = 2;

GSMetaTag(panel_init);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Init(panel* Panel, app_state* State)
{
    // TODO: :FreePanelMemory
    animation_timeline_state* TimelineState = PushStruct(&State->Permanent, animation_timeline_state);
    TimelineState->VisibleFrameRangeMin = State->AnimationSystem.StartFrame;
    TimelineState->VisibleFrameRangeMax = State->AnimationSystem.EndFrame;
    Panel->PanelStateMemory = (u8*)TimelineState;
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Cleanup(panel* Panel, app_state* State)
{
    
}

internal void
DrawFrameBar (animation_system* AnimationSystem, render_command_buffer* RenderBuffer, timeline_frame_range VisibleFrames, rect BarBounds, mouse_state Mouse, app_state* State)
{
    MakeStringBuffer(TempString, 256);
    
    s32 VisibleFrameCount = VisibleFrames.FrameMax - VisibleFrames.FrameMin;
    
    r32 BarHeight = Height(BarBounds);
    r32 BarWidth = Width(BarBounds);
    
    PushRenderQuad2D(RenderBuffer, RectExpand(BarBounds), v4{.16f, .16f, .16f, 1.f});
    
    // Mouse clicked inside frame nubmer bar -> change current frame on timeline
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState) && 
        PointIsInRange(Mouse.DownPos, RectExpand(BarBounds)))
    {
        StartDragTimeMarker(BarBounds, VisibleFrames, State);
    }
    
    // Frame Ticks
    u32 TickCount = 10;
    for (u32 Tick = 0; Tick < TickCount; Tick++)
    {
        r32 Percent = (r32)Tick / (r32)TickCount;
        u32 Frame = PercentToFrameInRange(Percent, VisibleFrames);
        PrintF(&TempString, "%d", Frame);
        r32 FramePercent = FrameToPercentRange(Frame, VisibleFrames);
        r32 FrameX = GSLerp(BarBounds.Min.x, BarBounds.Max.x, FramePercent);
        v2 FrameTextPos = v2{FrameX, BarBounds.Min.y + 2};
        DrawString(RenderBuffer, TempString, State->Interface.Font, FrameTextPos, WhiteV4);
        // Frame Vertical Slices
        v2 LineTop = v2{FrameX, BarBounds.Min.y};
        v2 LineBottom = v2{FrameX + 1, BarBounds.Min.y};
        PushRenderQuad2D(RenderBuffer, LineTop, LineBottom, v4{.16f, .16f, .16f, 1.f});
    }
    
    // Time Slider
    if (FrameIsInRange(AnimationSystem->CurrentFrame, VisibleFrames))
    { 
        r32 FrameAtPercentVisibleRange = FrameToPercentRange(AnimationSystem->CurrentFrame, VisibleFrames);
        r32 SliderX = GSLerp(BarBounds.Min.x, BarBounds.Max.x, FrameAtPercentVisibleRange);
        
        u32 FrameNumberCharCount = GetU32NumberOfCharactersNeeded(AnimationSystem->CurrentFrame);
        // space for each character + a margin on either side
        r32 SliderWidth = (8 * FrameNumberCharCount) + 8;
        r32 SliderHalfWidth = SliderWidth / 2.f;
        v2 HeadMin = v2{SliderX - SliderHalfWidth, BarBounds.Min.y};
        v2 HeadMax = v2{SliderX + SliderHalfWidth, BarBounds.Max.y};
        PushRenderQuad2D(RenderBuffer, HeadMin, HeadMax, TimeSliderColor);
        
        PrintF(&TempString, "%d", AnimationSystem->CurrentFrame);
        DrawString(RenderBuffer, TempString, State->Interface.Font, HeadMin + v2{6, 4}, WhiteV4);
    }
}

internal timeline_frame_range
DrawTimelineRangeBar (animation_system* AnimationSystem, animation_timeline_state* TimelineState, render_command_buffer* RenderBuffer, rect BarBounds, mouse_state Mouse)
{
    timeline_frame_range Result = {0};
    
    r32 BarHeight = Height(BarBounds);
    r32 BarWidth = Width(BarBounds);
    PushRenderQuad2D(RenderBuffer, RectExpand(BarBounds), v4{.16f, .16f, .16f, 1.f});
    
    r32 PlayableFrames = (r32)(AnimationSystem->EndFrame - AnimationSystem->StartFrame);
    v2 SliderBarDim = v2{25, BarHeight};
    
    // Convert Frames To Pixels
    // TODO(Peter): When the animation system is storing a frame range rather than individual values
    // come back and use the utility functions here
    r32 RangeMinPercentPlayable = (r32)(TimelineState->VisibleFrameRangeMin - AnimationSystem->StartFrame) / PlayableFrames;
    r32 RangeMaxPercentPlayable = (r32)(TimelineState->VisibleFrameRangeMax - AnimationSystem->StartFrame) / PlayableFrames;
    v2 RangeMinSliderMin = v2{BarBounds.Min.x + (RangeMinPercentPlayable * Width(BarBounds)), BarBounds.Min.y};
    v2 RangeMaxSliderMin = v2{BarBounds.Min.x + (RangeMaxPercentPlayable * Width(BarBounds)) - 25, BarBounds.Min.y};
    
    if (MouseButtonHeldDown(Mouse.LeftButtonState) || 
        MouseButtonTransitionedUp(Mouse.LeftButtonState))
    {
        v2 MouseDragOffset = Mouse.Pos - Mouse.DownPos;
        if (PointIsInRange(Mouse.DownPos, RangeMinSliderMin, RangeMinSliderMin + SliderBarDim))
        {
            r32 NewSliderX = RangeMinSliderMin.x + MouseDragOffset.x;
            RangeMinSliderMin.x = GSClamp(BarBounds.Min.x, NewSliderX, RangeMaxSliderMin.x - SliderBarDim.x);
        }
        if (PointIsInRange(Mouse.DownPos, RangeMaxSliderMin, RangeMaxSliderMin + SliderBarDim))
        {
            r32 NewSliderX = RangeMaxSliderMin.x + MouseDragOffset.x;
            RangeMaxSliderMin.x = GSClamp(RangeMinSliderMin.x + SliderBarDim.x, NewSliderX, BarBounds.Max.x - SliderBarDim.x);
        }
    }
    
    v2 RangeMinSliderMax = v2{RangeMinSliderMin.x + 25, BarBounds.Max.y};
    v2 RangeMaxSliderMax = v2{RangeMaxSliderMin.x + 25, BarBounds.Max.y};
    PushRenderQuad2D(RenderBuffer, RangeMinSliderMin, RangeMinSliderMax, v4{.8f, .8f, .8f, 1.f});
    PushRenderQuad2D(RenderBuffer, RangeMaxSliderMin, RangeMaxSliderMax, v4{.8f, .8f, .8f, 1.f});
    
    // Convert Pixels Back To Frames and store
    RangeMinPercentPlayable = (RangeMinSliderMin.x - BarBounds.Min.x) / BarWidth;
    RangeMaxPercentPlayable = (RangeMaxSliderMax.x - BarBounds.Min.x) / BarWidth;
    u32 VisibleFrameCount = AnimationSystem->EndFrame - AnimationSystem->StartFrame;
    Result.FrameMin = RangeMinPercentPlayable * VisibleFrameCount;
    Result.FrameMax = RangeMaxPercentPlayable * VisibleFrameCount;
    
    if (MouseButtonTransitionedUp(Mouse.LeftButtonState))
    {
        TimelineState->VisibleFrameRangeMin = Result.FrameMin;
        TimelineState->VisibleFrameRangeMax = Result.FrameMax;
    }
    
    return Result;
}

internal rect
DrawAnimationBlock (animation_block AnimationBlock, v4 BlockColor, timeline_frame_range VisibleFrames, rect TimelineBounds, render_command_buffer* RenderBuffer)
{
    rect BlockBounds = {};
    
    r32 TimelineWidth = Width(TimelineBounds);
    
    u32 ClampedBlockStartFrame = ClampFrameToRange(AnimationBlock.StartFrame, VisibleFrames);
    r32 StartFramePercent = FrameToPercentRange(ClampedBlockStartFrame, VisibleFrames);
    r32 StartPosition = TimelineWidth * StartFramePercent;
    
    u32 ClampedBlockEndFrame = ClampFrameToRange(AnimationBlock.EndFrame, VisibleFrames);
    r32 EndFramePercent = FrameToPercentRange(ClampedBlockEndFrame, VisibleFrames);
    r32 EndPosition = TimelineWidth * EndFramePercent;
    
    BlockBounds.Min = TimelineBounds.Min + v2{StartPosition, 25};
    BlockBounds.Max = TimelineBounds.Min + v2{EndPosition, 75};
    
    PushRenderQuad2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, BlockColor);
    PushRenderBoundingBox2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, 1, WhiteV4);
    
    return BlockBounds;
}

internal gs_list_handle
DrawAnimationTimeline (animation_system* AnimationSystem, animation_timeline_state* TimelineState, rect PanelBounds, gs_list_handle SelectedBlockHandle, render_command_buffer* RenderBuffer, app_state* State, mouse_state Mouse)
{
    string TempString = MakeString(PushArray(&State->Transient, char, 256), 256);
    gs_list_handle Result = SelectedBlockHandle;
    
    r32 AnimationPanelHeight = PanelBounds.Max.y - PanelBounds.Min.y;
    r32 AnimationPanelWidth = PanelBounds.Max.x - PanelBounds.Min.x;
    
    rect TimeRangeBarBounds = {0};
    TimeRangeBarBounds.Min = PanelBounds.Min;
    TimeRangeBarBounds.Max = { PanelBounds.Max.x, PanelBounds.Min.y + 24 };
    
    rect FrameBarBounds = {0};
    FrameBarBounds.Min = { PanelBounds.Min.x, PanelBounds.Max.y - 32 };
    FrameBarBounds.Max = PanelBounds.Max;
    
    rect TimelineBounds = {0};
    TimelineBounds.Min = TopLeft(TimeRangeBarBounds);
    TimelineBounds.Max = BottomRight(FrameBarBounds);
    
    timeline_frame_range AdjustedViewRange = {0};
    AdjustedViewRange = DrawTimelineRangeBar(AnimationSystem, TimelineState, RenderBuffer, TimeRangeBarBounds, Mouse);
    s32 VisibleFrameCount = AdjustedViewRange.FrameMax - AdjustedViewRange.FrameMin;
    
    DrawFrameBar(AnimationSystem, RenderBuffer, AdjustedViewRange, FrameBarBounds, Mouse, State);
    
    // Animation Blocks
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Mouse.LeftButtonState);
    for (u32 i = 0; i < AnimationSystem->Blocks.Used; i++)
    {
        gs_list_entry<animation_block>* AnimationBlockEntry = AnimationSystem->Blocks.GetEntryAtIndex(i);
        if (AnimationBlockEntry->Free.NextFreeEntry != 0) { continue; }
        
        gs_list_handle CurrentBlockHandle = AnimationBlockEntry->Handle;
        animation_block AnimationBlockAt = AnimationBlockEntry->Value;
        
        // If either end is in the range, we should draw it
        b32 RangeIsVisible = (FrameIsInRange(AnimationBlockAt.StartFrame, AdjustedViewRange) ||
                              FrameIsInRange(AnimationBlockAt.EndFrame, AdjustedViewRange));
        // If neither end is in the range, but the ends surround the visible range, 
        // we should still draw it.
        RangeIsVisible |= (AnimationBlockAt.StartFrame <= AdjustedViewRange.FrameMin && 
                           AnimationBlockAt.EndFrame >= AdjustedViewRange.FrameMax);
        if (RangeIsVisible)
        {
            v4 BlockColor = BlackV4;
            if (GSListHandlesAreEqual(SelectedBlockHandle, CurrentBlockHandle))
            {
                BlockColor = PinkV4;
            }
            rect BlockBounds = DrawAnimationBlock(AnimationBlockAt, BlockColor, AdjustedViewRange, TimelineBounds, RenderBuffer);
            if (PointIsInRange(Mouse.Pos, BlockBounds.Min, BlockBounds.Max)
                && MouseButtonTransitionedDown(Mouse.LeftButtonState))
            {
                MouseDownAndNotHandled = false;
                SelectAndBeginDragAnimationBlock(CurrentBlockHandle, AdjustedViewRange, TimelineBounds, State);
            }
        }
    }
    
    // Time Slider
    if (FrameIsInRange(AnimationSystem->CurrentFrame, AdjustedViewRange))
    {
        r32 FrameAtPercentVisibleRange = FrameToPercentRange(AnimationSystem->CurrentFrame, AdjustedViewRange);
        r32 SliderX = GSLerp(TimelineBounds.Min.x, TimelineBounds.Max.x, FrameAtPercentVisibleRange);
        v2 SliderMin = v2{SliderX, TimelineBounds.Min.y};
        v2 SliderMax = v2{SliderX + 1, TimelineBounds.Max.y};
        PushRenderQuad2D(RenderBuffer, SliderMin, SliderMax, TimeSliderColor);
    }
    
    PushRenderBoundingBox2D(RenderBuffer, RectExpand(TimeRangeBarBounds), 1.f, RedV4);
    PushRenderBoundingBox2D(RenderBuffer, RectExpand(FrameBarBounds), 1.f, TealV4);
    PushRenderBoundingBox2D(RenderBuffer, RectExpand(TimelineBounds), 1.f, PinkV4);
    
    return Result;
    
    
    
    {
        r32 FirstPlayablePercentX = FrameToPercentRange(AnimationSystem->StartFrame, AdjustedViewRange);
        r32 LastPlayablePercentX = FrameToPercentRange(AnimationSystem->EndFrame, AdjustedViewRange);
        
        v2 PlayableMin = v2{(FirstPlayablePercentX * AnimationPanelWidth) + PanelBounds.Min.x, PanelBounds.Min.y };
        v2 PlayableMax = v2{(LastPlayablePercentX * AnimationPanelWidth) + PanelBounds.Min.x, PanelBounds.Max.y };
        
        PushRenderQuad2D(RenderBuffer, PanelBounds.Min, PanelBounds.Max, v4{.16f, .16f, .16f, 1.f});
        PushRenderQuad2D(RenderBuffer, PlayableMin, PlayableMax, v4{.22f, .22f, .22f, 1.f});
    }
    
    if (MouseDownAndNotHandled && PointIsInRect(Mouse.Pos, TimelineBounds))
    {
        DeselectCurrentAnimationBlock(State);
    }
    return Result;
}

struct animation_clip
{
    char* Name;
    s32 NameLength;
    animation_proc* Proc;
};

s32 GlobalAnimationClipsCount = 3;
animation_clip GlobalAnimationClips[] = {
    { "Test Pattern One", 16, TestPatternOne  },
    { "Test Pattern Two", 16, TestPatternTwo },
    { "Test Pattern Three", 18, TestPatternThree },
};

internal void
DrawAnimationClipsList(rect PanelBounds, mouse_state Mouse, render_command_buffer* RenderBuffer, app_state* State)
{
    v4 LineBGColors[] = {
        { .16f, .16f, .16f, 1.f },
        { .18f, .18f, .18f, 1.f },
    };
    
    interface_list List = {};
    
    List.LineBGColors = LineBGColors;
    List.LineBGColorsCount = sizeof(LineBGColors) / sizeof(LineBGColors[0]);
    List.LineBGHoverColor = v4{ .22f, .22f, .22f, 1.f };
    List.TextColor = WhiteV4;
    List.ListBounds = PanelBounds;
    List.ListElementDimensions = v2{
        Width(PanelBounds), 
        (r32)(State->Interface.Font->PixelHeight + 8),
    };
    List.ElementLabelIndent = v2{10, 4};
    
    string TitleString = MakeStringLiteral("Animation Clips");
    DrawListElement(TitleString, &List, Mouse, RenderBuffer, State->Interface);
    
    for (s32 i = 0; i < GlobalAnimationClipsCount; i++)
    {
        animation_clip Clip = GlobalAnimationClips[i];
        string ClipName = MakeString(Clip.Name, Clip.NameLength);
        rect ElementBounds = DrawListElement(ClipName, &List, Mouse, RenderBuffer, State->Interface);
        
        if (MouseButtonTransitionedDown(Mouse.LeftButtonState) 
            && PointIsInRect(Mouse.DownPos, ElementBounds))
        {
            AddAnimationBlockAtCurrentTime(i + 1, &State->AnimationSystem);
        }
    }
    
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    animation_timeline_state* TimelineState = (animation_timeline_state*)Panel.PanelStateMemory;
    
    gs_list_handle SelectedBlockHandle = State->SelectedAnimationBlockHandle;
    
    r32 OptionsRowHeight = 25;
    rect AnimationClipListBounds = rect{
        PanelBounds.Min,
        v2{PanelBounds.Min.x + 300, PanelBounds.Max.y - OptionsRowHeight},
    };
    rect TimelineBounds = rect{
        v2{AnimationClipListBounds.Max.x, PanelBounds.Min.y},
        v2{PanelBounds.Max.x, PanelBounds.Max.y - OptionsRowHeight},
    };
    
    if (Height(TimelineBounds) > 0)
    {
        SelectedBlockHandle = DrawAnimationTimeline(&State->AnimationSystem, 
                                                    TimelineState,
                                                    TimelineBounds,
                                                    SelectedBlockHandle, 
                                                    RenderBuffer, State, Mouse);
        DrawAnimationClipsList(AnimationClipListBounds, Mouse, RenderBuffer, State);
    }
    
    v2 OptionsRowMin = v2{ PanelBounds.Min.x, TimelineBounds.Max.y };
    v2 OptionsRowMax = PanelBounds.Max;
    panel_result AnimationPanel = EvaluatePanel(RenderBuffer, OptionsRowMin, OptionsRowMax, State->Interface);
    
    r32 ButtonWidth = 35;
    v2 ButtonMin = v2{0, 0};
    v2 ButtonMax = v2{35, OptionsRowHeight - 2};
    v2 ButtonAt = v2{OptionsRowMin.x + 1, OptionsRowMin.y + 1};
    
    button_result PauseResult = EvaluateButton(RenderBuffer, 
                                               ButtonAt + ButtonMin, ButtonAt + ButtonMax,
                                               MakeStringLiteral("Pause"),
                                               State->Interface, Mouse);
    ButtonAt.x += ButtonWidth + 2;
    button_result PlayResult = EvaluateButton(RenderBuffer, 
                                              ButtonAt + ButtonMin, ButtonAt + ButtonMax,
                                              MakeStringLiteral("Play"),
                                              State->Interface, Mouse);
    ButtonAt.x += ButtonWidth + 2;
    button_result StopResult = EvaluateButton(RenderBuffer, 
                                              ButtonAt + ButtonMin, ButtonAt + ButtonMax,
                                              MakeStringLiteral("Stop"),
                                              State->Interface, Mouse);
    
    if (PauseResult.Pressed)
    {
        State->AnimationSystem.TimelineShouldAdvance = false;
    }
    
    if (PlayResult.Pressed)
    {
        State->AnimationSystem.TimelineShouldAdvance = true;
    }
    
    if (StopResult.Pressed)
    {
        State->AnimationSystem.TimelineShouldAdvance = false;
        State->AnimationSystem.CurrentFrame = 0;
    }
}

#define FOLDHAUS_PANEL_ANIMATION_TIMELINE_H
#endif // FOLDHAUS_PANEL_ANIMATION_TIMELINE_H