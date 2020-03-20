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
    frame_range VisibleRange;
};

inline u32
GetFrameFromPointInAnimationPanel(v2 Point, rect PanelBounds, frame_range VisibleRange)
{
    r32 HorizontalPercentOfBounds = (Point.x - PanelBounds.Min.x) / (PanelBounds.Max.x - PanelBounds.Min.x);
    u32 VisibleFramesCount = GetFrameCount(VisibleRange);
    u32 TimeAtPoint = (u32)(HorizontalPercentOfBounds * VisibleFramesCount) + VisibleRange.Min;
    return TimeAtPoint;
}

inline s32
GetXPositionFromFrameInAnimationPanel (u32 Frame, rect PanelBounds, frame_range VisibleRange)
{
    r32 PercentOfTimeline = (r32)(Frame - VisibleRange.Min) / (r32)GetFrameCount(VisibleRange);
    s32 XPositionAtFrame = (PercentOfTimeline * Width(PanelBounds)) + PanelBounds.Min.x;
    return XPositionAtFrame;
}

internal gs_list_handle
AddAnimationBlockAtCurrentTime (u32 AnimationProcHandle, u32 Layer, animation_system* System)
{
    u32 NewBlockStart = System->CurrentFrame;
    u32 NewBlockEnd = NewBlockStart + SecondsToFrames(3, *System);
    gs_list_handle Result = AddAnimationBlock(NewBlockStart, NewBlockEnd, AnimationProcHandle, Layer, System);
    return Result;
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
        RemoveAnimationBlock(State->SelectedAnimationBlockHandle, &State->AnimationSystem);
        State->SelectedAnimationBlockHandle = {0};
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
    frame_range Range = { OpState->StartFrame, OpState->EndFrame };
    u32 FrameAtMouseX = GetFrameFromPointInAnimationPanel(Mouse.Pos, OpState->TimelineBounds, Range);
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
StartDragTimeMarker(rect TimelineBounds, frame_range VisibleFrames, app_state* State)
{
    operation_mode* DragTimeMarkerMode = ActivateOperationModeWithCommands(&State->Modes, DragTimeMarkerCommands, UpdateDragTimeMarker);
    
    drag_time_marker_operation_state* OpState = CreateOperationState(DragTimeMarkerMode,
                                                                     &State->Modes,
                                                                     drag_time_marker_operation_state);
    OpState->StartFrame = VisibleFrames.Min;
    OpState->EndFrame = VisibleFrames.Max;
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
    frame_range VisibleRange;
    frame_range ClipRange;
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
    
    r32 ClipInitialStartFrameXPercent = FrameToPercentRange(OpState->ClipRange.Min, OpState->VisibleRange);
    u32 ClipInitialStartFrameXPosition = GSLerp(OpState->TimelineBounds.Min.x,
                                                OpState->TimelineBounds.Max.x,
                                                ClipInitialStartFrameXPercent);
    r32 ClipInitialEndFrameXPercent = FrameToPercentRange(OpState->ClipRange.Max, OpState->VisibleRange);
    u32 ClipInitialEndFrameXPosition = GSLerp(OpState->TimelineBounds.Min.x,
                                              OpState->TimelineBounds.Max.x,
                                              ClipInitialEndFrameXPercent);
    
    u32 FrameAtMouseDownX = GetFrameFromPointInAnimationPanel(Mouse.DownPos, OpState->TimelineBounds, OpState->VisibleRange);
    
    u32 FrameAtMouseX = GetFrameFromPointInAnimationPanel(Mouse.Pos, OpState->TimelineBounds, OpState->VisibleRange);
    s32 FrameOffset = (s32)FrameAtMouseX - (s32)FrameAtMouseDownX;
    
    animation_block* AnimationBlock = State->AnimationSystem.Blocks.GetElementWithHandle(State->SelectedAnimationBlockHandle);
    if (!AnimationBlock)
    {
        EndCurrentOperationMode(State, {}, Mouse);
        return;
    }
    
    if (GSAbs(Mouse.DownPos.x - ClipInitialStartFrameXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        s32 NewStartFrame = OpState->ClipRange.Min + FrameOffset;
        if (FrameOffset < 0)
        {
            for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
            {
                gs_list_entry<animation_block>* OtherBlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
                if (EntryIsFree(OtherBlockEntry)) { continue; }
                animation_block OtherBlock = OtherBlockEntry->Value;
                NewStartFrame = AttemptToSnapPosition(NewStartFrame, OtherBlock.Range.Max);
            }
        }
        else
        {
            if (NewStartFrame >= AnimationBlock->Range.Max)
            {
                NewStartFrame = AnimationBlock->Range.Max - 1;
            }
        }
        AnimationBlock->Range.Min = NewStartFrame; 
    }
    else if (GSAbs(Mouse.DownPos.x - ClipInitialEndFrameXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        r32 NewEndFrame = OpState->ClipRange.Max + FrameOffset;
        if (FrameOffset > 0)
        {
            for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
            {
                gs_list_entry<animation_block>* OtherBlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
                if (EntryIsFree(OtherBlockEntry)) { continue; }
                animation_block OtherBlock = OtherBlockEntry->Value;
                NewEndFrame = AttemptToSnapPosition(NewEndFrame, OtherBlock.Range.Min);
            }
        }
        else
        {
            if(NewEndFrame <= AnimationBlock->Range.Min)
            {
                NewEndFrame = AnimationBlock->Range.Min + 1;
            }
        }
        AnimationBlock->Range.Max = NewEndFrame;
    }
    else
    {
        u32 NewStartFrame = OpState->ClipRange.Min + FrameOffset;
        u32 NewEndFrame = OpState->ClipRange.Max + FrameOffset;
        for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
        {
            gs_list_entry<animation_block>* OtherBlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
            if (EntryIsFree(OtherBlockEntry)) { continue; }
            animation_block OtherBlock = OtherBlockEntry->Value;
            
            u32 SnapFramesAmount = 0;
            if (FrameOffset > 0)
            {
                u32 FinalEndFrame = AttemptToSnapPosition(NewEndFrame, OtherBlock.Range.Min);
                SnapFramesAmount = FinalEndFrame - NewEndFrame;
            }
            else if (FrameOffset < 0)
            {
                u32 FinalStartFrame = AttemptToSnapPosition(NewStartFrame, OtherBlock.Range.Max);
                SnapFramesAmount = FinalStartFrame - NewStartFrame;
            }
            NewEndFrame += SnapFramesAmount;
            NewStartFrame += SnapFramesAmount;
        }
        AnimationBlock->Range.Min = NewStartFrame;
        AnimationBlock->Range.Max = NewEndFrame;
    }
    
    s32 PlayableStartFrame = State->AnimationSystem.PlayableRange.Min;
    s32 PlayableEndFrame = State->AnimationSystem.PlayableRange.Max;
    AnimationBlock->Range.Min = (u32)GSClamp(PlayableStartFrame, (s32)AnimationBlock->Range.Min, PlayableEndFrame);
    AnimationBlock->Range.Max = (u32)GSClamp(PlayableStartFrame, (s32)AnimationBlock->Range.Max, PlayableEndFrame);
}

input_command DragAnimationClipCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndCurrentOperationMode },
};

internal void
SelectAndBeginDragAnimationBlock(gs_list_handle BlockHandle, frame_range VisibleRange, rect TimelineBounds, app_state* State)
{
    SelectAnimationBlock(BlockHandle, State);
    
    operation_mode* DragAnimationClipMode = ActivateOperationModeWithCommands(&State->Modes, DragAnimationClipCommands, UpdateDragAnimationClip);
    
    drag_animation_clip_state* OpState = CreateOperationState(DragAnimationClipMode,
                                                              &State->Modes,
                                                              drag_animation_clip_state);
    OpState->TimelineBounds = TimelineBounds;
    OpState->VisibleRange = VisibleRange;
    
    animation_block* SelectedBlock = State->AnimationSystem.Blocks.GetElementWithHandle(BlockHandle);
    OpState->ClipRange = SelectedBlock->Range;
}
// -------------------

FOLDHAUS_INPUT_COMMAND_PROC(AddAnimationBlockCommand)
{
    panel_and_bounds ActivePanel = GetPanelContainingPoint(Mouse.Pos, &State->PanelSystem, State->WindowBounds);
    frame_range Range = State->AnimationSystem.PlayableRange; 
    u32 MouseDownFrame = GetFrameFromPointInAnimationPanel(Mouse.Pos, ActivePanel.Bounds, Range);
    gs_list_handle NewBlockHandle = AddAnimationBlock(MouseDownFrame, MouseDownFrame + SecondsToFrames(3, State->AnimationSystem), 4, State->SelectedAnimationLayer, &State->AnimationSystem);
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
    TimelineState->VisibleRange = State->AnimationSystem.PlayableRange;
    Panel->PanelStateMemory = (u8*)TimelineState;
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Cleanup(panel* Panel, app_state* State)
{
    
}

internal void
DrawFrameBar (animation_system* AnimationSystem, render_command_buffer* RenderBuffer, frame_range VisibleFrames, rect BarBounds, mouse_state Mouse, app_state* State)
{
    MakeStringBuffer(TempString, 256);
    
    s32 VisibleFrameCount = VisibleFrames.Max - VisibleFrames.Min;
    
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

internal frame_range
DrawTimelineRangeBar (animation_system* AnimationSystem, animation_timeline_state* TimelineState, render_command_buffer* RenderBuffer, rect BarBounds, mouse_state Mouse)
{
    frame_range Result = {0};
    
    r32 BarHeight = Height(BarBounds);
    r32 BarWidth = Width(BarBounds);
    PushRenderQuad2D(RenderBuffer, RectExpand(BarBounds), v4{.16f, .16f, .16f, 1.f});
    
    r32 PlayableFrames = (r32)GetFrameCount(AnimationSystem->PlayableRange); 
    v2 SliderBarDim = v2{25, BarHeight};
    
    // Convert Frames To Pixels
    r32 VisibleMinPercentPlayable = FrameToPercentRange(TimelineState->VisibleRange.Min, AnimationSystem->PlayableRange);
    r32 VisibleMaxPercentPlayable = FrameToPercentRange(TimelineState->VisibleRange.Max, AnimationSystem->PlayableRange);
    v2 RangeMinSliderMin = v2{BarBounds.Min.x + (VisibleMinPercentPlayable * Width(BarBounds)), BarBounds.Min.y};
    v2 RangeMaxSliderMin = v2{BarBounds.Min.x + (VisibleMaxPercentPlayable * Width(BarBounds)) - 25, BarBounds.Min.y};
    
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
    VisibleMinPercentPlayable = (RangeMinSliderMin.x - BarBounds.Min.x) / BarWidth;
    VisibleMaxPercentPlayable = (RangeMaxSliderMax.x - BarBounds.Min.x) / BarWidth;
    u32 VisibleFrameCount = GetFrameCount(AnimationSystem->PlayableRange);
    Result.Min = VisibleMinPercentPlayable * VisibleFrameCount;
    Result.Max = VisibleMaxPercentPlayable * VisibleFrameCount;
    
    if (MouseButtonTransitionedUp(Mouse.LeftButtonState)) 
    {
        TimelineState->VisibleRange = Result;
    }
    
    return Result;
}

#define LAYER_HEIGHT 52

internal void
DrawLayerMenu(animation_system* AnimationSystem, rect PanelDim, render_command_buffer* RenderBuffer, app_state* State, mouse_state Mouse)
{
    v2 LayerDim = { Width(PanelDim), LAYER_HEIGHT };
    v2 LayerListMin = PanelDim.Min + v2{0, 24};
    for (u32 LayerIndex = 0; LayerIndex < AnimationSystem->LayersCount; LayerIndex++)
    {
        anim_layer* Layer = AnimationSystem->Layers + LayerIndex;
        rect LayerBounds = {0};
        LayerBounds.Min = { LayerListMin.x, LayerListMin.y + (LayerDim.y * LayerIndex) };
        LayerBounds.Max = LayerBounds.Min + LayerDim;
        
        if (MouseButtonTransitionedDown(Mouse.LeftButtonState) &&
            PointIsInRect(Mouse.Pos, LayerBounds))
        {
            State->SelectedAnimationLayer = LayerIndex;
        }
        
        v2 LayerTextPos = { LayerBounds.Min.x + 6, LayerBounds.Max.y - 16};
        if (State->SelectedAnimationLayer == LayerIndex)
        {
            PushRenderBoundingBox2D(RenderBuffer, RectExpand(LayerBounds), 1, WhiteV4);
        }
        DrawString(RenderBuffer, Layer->Name, State->Interface.Font, LayerTextPos, WhiteV4);
    }
}

internal rect
DrawAnimationBlock (animation_block AnimationBlock, v4 BlockColor, frame_range VisibleFrames, rect TimelineBounds, render_command_buffer* RenderBuffer)
{
    rect BlockBounds = {};
    
    r32 TimelineWidth = Width(TimelineBounds);
    
    u32 ClampedBlockStartFrame = ClampFrameToRange(AnimationBlock.Range.Min, VisibleFrames);
    r32 StartFramePercent = FrameToPercentRange(ClampedBlockStartFrame, VisibleFrames);
    r32 StartPosition = TimelineWidth * StartFramePercent;
    
    u32 ClampedBlockEndFrame = ClampFrameToRange(AnimationBlock.Range.Max, VisibleFrames);
    r32 EndFramePercent = FrameToPercentRange(ClampedBlockEndFrame, VisibleFrames);
    r32 EndPosition = TimelineWidth * EndFramePercent;
    
    r32 LayerYOffset = LAYER_HEIGHT * AnimationBlock.Layer;
    BlockBounds.Min = TimelineBounds.Min + v2{StartPosition, LayerYOffset};
    BlockBounds.Max = TimelineBounds.Min + v2{EndPosition, LayerYOffset + LAYER_HEIGHT};
    
    PushRenderQuad2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, BlockColor);
    PushRenderBoundingBox2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, 1, WhiteV4);
    
    return BlockBounds;
}

internal gs_list_handle
DrawAnimationTimeline (animation_system* AnimationSystem, animation_timeline_state* TimelineState, rect PanelBounds, gs_list_handle SelectedBlockHandle, ui_interface* Interface, app_state* State)
{
    string TempString = MakeString(PushArray(&State->Transient, char, 256), 256);
    gs_list_handle Result = SelectedBlockHandle;
    
    rect LayerMenuBounds, TimelineBounds;
    VSplitRectAtDistanceFromLeft(PanelBounds, 256, &LayerMenuBounds, &TimelineBounds);
    
    // In Top To Bottom Order
    rect TimelineFrameBarBounds, TimelineBlockDisplayBounds, TimelineRangeBarBounds;
    HSplitRectAtDistanceFromTop(TimelineBounds, 32, &TimelineFrameBarBounds, &TimelineBounds);
    HSplitRectAtDistanceFromBottom(TimelineBounds, 24, &TimelineBlockDisplayBounds, &TimelineRangeBarBounds);
    
    // TODO(Peter): Clean Up
    DrawLayerMenu(AnimationSystem, LayerMenuBounds, Interface->RenderBuffer, State, Interface->Mouse);
    
    frame_range AdjustedViewRange = {0};
    // TODO(Peter): Clean Up
    AdjustedViewRange = DrawTimelineRangeBar(AnimationSystem, TimelineState, Interface->RenderBuffer, TimelineRangeBarBounds, Interface->Mouse);
    s32 VisibleFrameCount = AdjustedViewRange.Max - AdjustedViewRange.Min;
    
    // TODO(Peter): Clean Up
    DrawFrameBar(AnimationSystem, Interface->RenderBuffer, AdjustedViewRange, TimelineFrameBarBounds, Interface->Mouse, State);
    
    ui_FillRect(Interface, TimelineBlockDisplayBounds, v4{.25f, .25f, .25f, 1.0f});
    
    // Animation Blocks
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState);
    gs_list_handle DragBlockHandle = {0};
    for (u32 i = 0; i < AnimationSystem->Blocks.Used; i++)
    {
        gs_list_entry<animation_block>* AnimationBlockEntry = AnimationSystem->Blocks.GetEntryAtIndex(i);
        if (EntryIsFree(AnimationBlockEntry)) { continue; }
        
        gs_list_handle CurrentBlockHandle = AnimationBlockEntry->Handle;
        animation_block AnimationBlockAt = AnimationBlockEntry->Value;
        
        // If either end is in the range, we should draw it
        b32 RangeIsVisible = (FrameIsInRange(AnimationBlockAt.Range.Min, AdjustedViewRange) ||
                              FrameIsInRange(AnimationBlockAt.Range.Max, AdjustedViewRange));
        // If neither end is in the range, but the ends surround the visible range, 
        // we should still draw it.
        RangeIsVisible |= (AnimationBlockAt.Range.Min <= AdjustedViewRange.Min && 
                           AnimationBlockAt.Range.Max>= AdjustedViewRange.Max);
        if (RangeIsVisible)
        {
            v4 BlockColor = BlackV4;
            if (GSListHandlesAreEqual(SelectedBlockHandle, CurrentBlockHandle))
            {
                BlockColor = PinkV4;
            }
            // TODO(Peter): Clean Up
            rect BlockBounds = DrawAnimationBlock(AnimationBlockAt, BlockColor, AdjustedViewRange, TimelineBounds, Interface->RenderBuffer);
            if (PointIsInRect(Interface->Mouse.Pos, BlockBounds))
            {
                DragBlockHandle = CurrentBlockHandle;
            }
        }
    }
    
    if (MouseDownAndNotHandled && ListHandleIsValid(DragBlockHandle))
    {
        MouseDownAndNotHandled = false;
        // TODO(Peter): Why are we passing state around?
        SelectAndBeginDragAnimationBlock(DragBlockHandle, AdjustedViewRange, TimelineBounds, State);
    }
    
    // Time Slider
    if (FrameIsInRange(AnimationSystem->CurrentFrame, AdjustedViewRange))
    {
        r32 FrameAtPercentVisibleRange = FrameToPercentRange(AnimationSystem->CurrentFrame, AdjustedViewRange);
        r32 SliderX = GSLerp(TimelineBounds.Min.x, TimelineBounds.Max.x, FrameAtPercentVisibleRange);
        rect SliderBounds = {
            v2{ SliderX, TimelineBounds.Min.y },
            v2{ SliderX + 1, TimelineBounds.Max.y }
        };
        ui_FillRect(Interface, SliderBounds, TimeSliderColor);
    }
    
    ui_OutlineRect(Interface, TimelineRangeBarBounds, 1.f, RedV4);
    ui_OutlineRect(Interface, TimelineFrameBarBounds, 1.f, RedV4);
    ui_OutlineRect(Interface, TimelineBlockDisplayBounds, 1.f, RedV4);
    
    if (MouseDownAndNotHandled && PointIsInRect(Interface->Mouse.Pos, TimelineBounds))
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
DrawAnimationClipsList(rect PanelBounds, ui_interface* Interface, u32 SelectedAnimationLayer, animation_system* AnimationSystem)
{
    ui_layout Layout = ui_CreateLayout(*Interface, PanelBounds);
    for (s32 i = 0; i < GlobalAnimationClipsCount; i++)
    {
        animation_clip Clip = GlobalAnimationClips[i];
        string ClipName = MakeString(Clip.Name, Clip.NameLength);
        if (ui_LayoutListEntry(Interface, &Layout, ClipName, i))
        {
            AddAnimationBlockAtCurrentTime(i + 1, SelectedAnimationLayer, AnimationSystem);
        }
    }
    // TODO(Peter): Fill up the rest of the area with empty list entries
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    animation_timeline_state* TimelineState = (animation_timeline_state*)Panel.PanelStateMemory;
    gs_list_handle SelectedBlockHandle = State->SelectedAnimationBlockHandle;
    
    ui_interface* Interface = &State->Interface_;
    animation_system* AnimationSystem = &State->AnimationSystem;
    
    rect TitleBarBounds, PanelContentsBounds;
    HSplitRectAtDistanceFromTop(PanelBounds, Interface->Style.RowHeight, &TitleBarBounds, &PanelContentsBounds);
    rect AnimationListBounds, TimelineBounds;
    VSplitRectAtDistanceFromLeft(PanelContentsBounds, 300, &AnimationListBounds, &TimelineBounds);
    
    ui_FillRect(Interface, TitleBarBounds, Interface->Style.PanelBGColors[0]);
    ui_layout TitleBarLayout = ui_CreateLayout(*Interface, TitleBarBounds);
    ui_StartRow(&TitleBarLayout, 3);
    {
        if (ui_LayoutButton(Interface, MakeStringLiteral("Pause"), &TitleBarLayout))
        {
            State->AnimationSystem.TimelineShouldAdvance = true;
        }
        if (ui_LayoutButton(Interface, MakeStringLiteral("Play"), &TitleBarLayout))
        {
            State->AnimationSystem.TimelineShouldAdvance = false;
        }
        if (ui_LayoutButton(Interface, MakeStringLiteral("Stop"), &TitleBarLayout))
        {
            State->AnimationSystem.TimelineShouldAdvance = false;
            State->AnimationSystem.CurrentFrame = 0;
        }
    }
    ui_EndRow(&TitleBarLayout);
    
    if (Height(TimelineBounds) > 0)
    {
        SelectedBlockHandle = DrawAnimationTimeline(AnimationSystem, TimelineState, TimelineBounds, SelectedBlockHandle, Interface, State);
        DrawAnimationClipsList(AnimationListBounds, Interface, State->SelectedAnimationLayer, &State->AnimationSystem);
    }
}

#define FOLDHAUS_PANEL_ANIMATION_TIMELINE_H
#endif // FOLDHAUS_PANEL_ANIMATION_TIMELINE_H