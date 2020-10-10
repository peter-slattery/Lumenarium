//
// File: foldhaus_panel_animation_timeline.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_ANIMATION_TIMELINE_H

// Colors
global v4 TimeSliderColor = v4{.36f, .52f, .78f, 1.f};

//
struct animation_timeline_state
{
    frame_range VisibleRange;
    gs_list_handle SelectedAnimationBlockHandle;
    u32 SelectedAnimationLayer;
};

inline u32
GetFrameFromPointInAnimationPanel(v2 Point, rect2 PanelBounds, frame_range VisibleRange)
{
    r32 HorizontalPercentOfBounds = (Point.x - PanelBounds.Min.x) / (PanelBounds.Max.x - PanelBounds.Min.x);
    u32 VisibleFramesCount = GetFrameCount(VisibleRange);
    u32 TimeAtPoint = (u32)(HorizontalPercentOfBounds * VisibleFramesCount) + VisibleRange.Min;
    return TimeAtPoint;
}

inline s32
GetXPositionFromFrameInAnimationPanel (u32 Frame, rect2 PanelBounds, frame_range VisibleRange)
{
    r32 PercentOfTimeline = (r32)(Frame - VisibleRange.Min) / (r32)GetFrameCount(VisibleRange);
    s32 XPositionAtFrame = (PercentOfTimeline * Rect2Width(PanelBounds)) + PanelBounds.Min.x;
    return XPositionAtFrame;
}

internal gs_list_handle
AddAnimationBlockAtCurrentTime (u32 AnimationProcHandle, u32 LayerHandle, animation_system* System)
{
    u32 NewBlockStart = System->CurrentFrame;
    u32 NewBlockEnd = NewBlockStart + SecondsToFrames(3, *System);
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
    gs_list_handle Result = Animation_AddBlock(ActiveAnim, NewBlockStart, NewBlockEnd, AnimationProcHandle, LayerHandle);
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
        animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
        Animation_RemoveBlock(ActiveAnim, State->SelectedAnimationBlockHandle);
        State->SelectedAnimationBlockHandle = {0};
    }
}

//
// Drag Time Marker
//

OPERATION_STATE_DEF(drag_time_marker_operation_state)
{
    rect2 TimelineBounds;
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
StartDragTimeMarker(rect2 TimelineBounds, frame_range VisibleFrames, app_state* State)
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
    rect2 TimelineBounds;
    frame_range VisibleRange;
    frame_range ClipRange;
};

internal u32
AttemptToSnapPosition(u32 SnappingFrame, u32 SnapToFrame)
{
    u32 Result = SnappingFrame;
    s32 SnapDistance = 5;
    if (Abs((s32)SnappingFrame - (s32)SnapToFrame) <= SnapDistance)
    {
        Result = SnapToFrame;
    }
    return Result;
}

OPERATION_RENDER_PROC(UpdateDragAnimationClip)
{
    drag_animation_clip_state* OpState = (drag_animation_clip_state*)Operation.OpStateMemory;
    
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    
    r32 ClipInitialStartFrameXPercent = FrameToPercentRange(OpState->ClipRange.Min, OpState->VisibleRange);
    u32 ClipInitialStartFrameXPosition = LerpR32(ClipInitialStartFrameXPercent,
                                                 OpState->TimelineBounds.Min.x,
                                                 OpState->TimelineBounds.Max.x);
    r32 ClipInitialEndFrameXPercent = FrameToPercentRange(OpState->ClipRange.Max, OpState->VisibleRange);
    u32 ClipInitialEndFrameXPosition = LerpR32(ClipInitialEndFrameXPercent,
                                               OpState->TimelineBounds.Min.x,
                                               OpState->TimelineBounds.Max.x);
    
    u32 FrameAtMouseDownX = GetFrameFromPointInAnimationPanel(Mouse.DownPos, OpState->TimelineBounds, OpState->VisibleRange);
    
    u32 FrameAtMouseX = GetFrameFromPointInAnimationPanel(Mouse.Pos, OpState->TimelineBounds, OpState->VisibleRange);
    s32 FrameOffset = (s32)FrameAtMouseX - (s32)FrameAtMouseDownX;
    
    animation_block* AnimationBlock = ActiveAnim->Blocks.GetElementWithHandle(State->SelectedAnimationBlockHandle);
    if (!AnimationBlock)
    {
        EndCurrentOperationMode(State, {}, Mouse, Context);
        return;
    }
    
    if (Abs(Mouse.DownPos.x - ClipInitialStartFrameXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        s32 NewStartFrame = OpState->ClipRange.Min + FrameOffset;
        if (FrameOffset < 0)
        {
            for (u32 i = 0; i < ActiveAnim->Blocks.Used; i++)
            {
                gs_list_entry<animation_block>* OtherBlockEntry = ActiveAnim->Blocks.GetEntryAtIndex(i);
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
    else if (Abs(Mouse.DownPos.x - ClipInitialEndFrameXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        r32 NewEndFrame = OpState->ClipRange.Max + FrameOffset;
        if (FrameOffset > 0)
        {
            for (u32 i = 0; i < ActiveAnim->Blocks.Used; i++)
            {
                gs_list_entry<animation_block>* OtherBlockEntry = ActiveAnim->Blocks.GetEntryAtIndex(i);
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
        for (u32 i = 0; i < ActiveAnim->Blocks.Used; i++)
        {
            gs_list_entry<animation_block>* OtherBlockEntry = ActiveAnim->Blocks.GetEntryAtIndex(i);
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
    
    s32 PlayableStartFrame = ActiveAnim->PlayableRange.Min;
    s32 PlayableEndFrame = ActiveAnim->PlayableRange.Max;
    AnimationBlock->Range.Min = (u32)Clamp(PlayableStartFrame, (s32)AnimationBlock->Range.Min, PlayableEndFrame);
    AnimationBlock->Range.Max = (u32)Clamp(PlayableStartFrame, (s32)AnimationBlock->Range.Max, PlayableEndFrame);
}

input_command DragAnimationClipCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndCurrentOperationMode },
};

internal void
SelectAndBeginDragAnimationBlock(gs_list_handle BlockHandle, frame_range VisibleRange, rect2 TimelineBounds, app_state* State)
{
    SelectAnimationBlock(BlockHandle, State);
    
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    operation_mode* DragAnimationClipMode = ActivateOperationModeWithCommands(&State->Modes, DragAnimationClipCommands, UpdateDragAnimationClip);
    
    drag_animation_clip_state* OpState = CreateOperationState(DragAnimationClipMode,
                                                              &State->Modes,
                                                              drag_animation_clip_state);
    OpState->TimelineBounds = TimelineBounds;
    OpState->VisibleRange = VisibleRange;
    
    animation_block* SelectedBlock = ActiveAnim->Blocks.GetElementWithHandle(BlockHandle);
    OpState->ClipRange = SelectedBlock->Range;
}
// -------------------

FOLDHAUS_INPUT_COMMAND_PROC(AddAnimationBlockCommand)
{
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    
    panel_and_bounds ActivePanel = GetPanelContainingPoint(Mouse.Pos, &State->PanelSystem, State->WindowBounds);
    frame_range Range = ActiveAnim->PlayableRange;
    u32 MouseDownFrame = GetFrameFromPointInAnimationPanel(Mouse.Pos, ActivePanel.Bounds, Range);
    
    gs_list_handle NewBlockHandle = Animation_AddBlock(ActiveAnim, MouseDownFrame, MouseDownFrame + SecondsToFrames(3, State->AnimationSystem), 4, State->SelectedAnimationLayer);
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
AnimationTimeline_Init(panel* Panel, app_state* State, context Context)
{
    // TODO: :FreePanelMemory
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    animation_timeline_state* TimelineState = PushStruct(&State->Permanent, animation_timeline_state);
    TimelineState->VisibleRange = ActiveAnim->PlayableRange;
    Panel->PanelStateMemory = (u8*)TimelineState;
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Cleanup(panel* Panel, app_state* State)
{
    
}

internal void
DrawFrameBar (animation_system* AnimationSystem, ui_interface Interface, frame_range VisibleFrames, rect2 BarBounds, app_state* State)
{
    gs_string TempString = PushString(State->Transient, 256);
    
    s32 VisibleFrameCount = VisibleFrames.Max - VisibleFrames.Min;
    
    r32 BarHeight = Rect2Height(BarBounds);
    r32 BarWidth = Rect2Width(BarBounds);
    
    // Mouse clicked inside frame nubmer bar -> change current frame on timeline
    if (MouseButtonTransitionedDown(Interface.Mouse.LeftButtonState) &&
        PointIsInRect(BarBounds, Interface.Mouse.DownPos))
    {
        StartDragTimeMarker(BarBounds, VisibleFrames, State);
    }
    
    PushRenderQuad2D(Interface.RenderBuffer, BarBounds.Min, BarBounds.Max, v4{.16f, .16f, .16f, 1.f});
    
    // Frame Ticks
    u32 TickCount = 10;
    for (u32 Tick = 0; Tick < TickCount; Tick++)
    {
        r32 Percent = (r32)Tick / (r32)TickCount;
        u32 Frame = PercentToFrameInRange(Percent, VisibleFrames);
        PrintF(&TempString, "%d", Frame);
        r32 FramePercent = FrameToPercentRange(Frame, VisibleFrames);
        r32 FrameX = LerpR32(FramePercent, BarBounds.Min.x, BarBounds.Max.x);
        v2 FrameTextPos = v2{FrameX, BarBounds.Min.y + 2};
        DrawString(Interface.RenderBuffer, TempString, Interface.Style.Font, FrameTextPos, WhiteV4);
    }
    
    // Time Slider
    if (FrameIsInRange(VisibleFrames, AnimationSystem->CurrentFrame))
    {
        r32 FrameAtPercentVisibleRange = FrameToPercentRange(AnimationSystem->CurrentFrame, VisibleFrames);
        r32 SliderX = LerpR32(FrameAtPercentVisibleRange, BarBounds.Min.x, BarBounds.Max.x);
        
        PrintF(&TempString, "%d", AnimationSystem->CurrentFrame);
        
        // space for each character + a margin on either side
        r32 SliderWidth = (8 * TempString.Length) + 8;
        r32 SliderHalfWidth = SliderWidth / 2.f;
        v2 HeadMin = v2{SliderX - SliderHalfWidth, BarBounds.Min.y};
        v2 HeadMax = v2{SliderX + SliderHalfWidth, BarBounds.Max.y};
        PushRenderQuad2D(Interface.RenderBuffer, HeadMin, HeadMax, TimeSliderColor);
        DrawString(Interface.RenderBuffer, TempString, Interface.Style.Font, HeadMin + v2{6, 4}, WhiteV4);
    }
}

internal bool
MinMaxRangeSlider(v2 HandleValues, rect2 SliderBounds, r32 MinValue, r32 MaxValue, ui_interface Interface, v2* OutHandleValues)
{
    // Should Update only gets set to true when the user is finished interacting (ie. on mouse up)
    // this allows the continuous use of the value of a handle while it is being dragged, and allows
    // for you to know when exactly to update the stored value
    
    bool ShouldUpdate = false;
    *OutHandleValues = HandleValues;
    
    v4 BGColor = v4{.16f, .16f, .16f, 1.f};
    v4 HandleColor = v4{.8f, .8f, .8f, 1.f};
    
    v2 HandleDim = v2{25, Rect2Height(SliderBounds)};
    r32 MinHandleX = RemapR32(HandleValues.x, MinValue, MaxValue, SliderBounds.Min.x, SliderBounds.Max.x);
    r32 MaxHandleX = RemapR32(HandleValues.y, MinValue, MaxValue, SliderBounds.Min.x, SliderBounds.Max.x);
    rect2 MinHandleBounds = MakeRect2CenterDim(v2{ MinHandleX, Rect2Center(SliderBounds).y }, HandleDim);
    rect2 MaxHandleBounds = MakeRect2CenterDim(v2{ MaxHandleX, Rect2Center(SliderBounds).y }, HandleDim);
    
    // Drag the handles
    if (MouseButtonHeldDown(Interface.Mouse.LeftButtonState) ||
        MouseButtonTransitionedUp(Interface.Mouse.LeftButtonState))
    {
        v2 MouseDragOffset = Interface.Mouse.Pos - Interface.Mouse.DownPos;
        
        // TODO(pjs): We need to make sure that the min handle is always the lower one, etc.
        // TODO(pjs): We need to range clamp the handles
        if (PointIsInRect(MinHandleBounds, Interface.Mouse.DownPos))
        {
            MinHandleBounds = Rect2TranslateX(MinHandleBounds, MouseDragOffset.x);
        }
        else if (PointIsInRect(MaxHandleBounds, Interface.Mouse.DownPos))
        {
            MaxHandleBounds = Rect2TranslateX(MaxHandleBounds, MouseDragOffset.x);
        }
    }
	
    // Draw Background
    PushRenderQuad2D(Interface.RenderBuffer, SliderBounds.Min, SliderBounds.Max, BGColor);
    
    // Draw Handles
    PushRenderQuad2D(Interface.RenderBuffer, MinHandleBounds.Min, MinHandleBounds.Max, HandleColor);
    PushRenderQuad2D(Interface.RenderBuffer, MaxHandleBounds.Min, MaxHandleBounds.Max, HandleColor);
    
    // Update the output range value
    r32 MinHandleXOut = Rect2Center(MinHandleBounds).x;
    r32 MaxHandleXOut = Rect2Center(MaxHandleBounds).x;
    
    r32 MinHandleValue = RemapR32(MinHandleXOut, SliderBounds.Min.x, SliderBounds.Max.x, MinValue, MaxValue);
    r32 MaxHandleValue = RemapR32(MaxHandleXOut, SliderBounds.Min.x, SliderBounds.Max.x, MinValue, MaxValue);
    
    *OutHandleValues = v2{ Min(MinHandleValue, MaxHandleValue), Max(MinHandleValue, MaxHandleValue) };
    
    if (MouseButtonTransitionedUp(Interface.Mouse.LeftButtonState))
    {
        ShouldUpdate = true;
    }
    
    return ShouldUpdate;
}


internal frame_range
DrawTimelineRangeBar (animation_system* AnimationSystem, animation Animation, animation_timeline_state* TimelineState, ui_interface Interface, rect2 BarBounds)
{
    frame_range VisibleRangeAfterInteraction = {};
    r32 MinFrame = (r32)Animation.PlayableRange.Min;
    r32 MaxFrame = (r32)Animation.PlayableRange.Max;
    
    v2 RangeHandles = v2{ (r32)TimelineState->VisibleRange.Min, (r32)TimelineState->VisibleRange.Max };
    
    bool ApplyUpdate = MinMaxRangeSlider(RangeHandles, BarBounds, MinFrame, MaxFrame, Interface, &RangeHandles);
    VisibleRangeAfterInteraction.Min = (s32)RangeHandles.x;
    VisibleRangeAfterInteraction.Max = (s32)RangeHandles.y;
    
    if (ApplyUpdate)
    {
        TimelineState->VisibleRange = VisibleRangeAfterInteraction;
    }
    
    return VisibleRangeAfterInteraction;
}

#define LAYER_HEIGHT 52

internal void
DrawLayerMenu(animation_system* AnimationSystem, animation ActiveAnim, ui_interface Interface, rect2 PanelDim, u32* SelectedAnimationLayer)
{
    v2 LayerDim = { Rect2Width(PanelDim), LAYER_HEIGHT };
    v2 LayerListMin = PanelDim.Min + v2{0, 24};
    for (u32 i = 0; i < ActiveAnim.Layers.Count; i++)
    {
        anim_layer* Layer = ActiveAnim.Layers.Values + i;
        
        rect2 LayerBounds = {0};
        LayerBounds.Min = { LayerListMin.x, LayerListMin.y + (LayerDim.y * i) };
        LayerBounds.Max = LayerBounds.Min + LayerDim;
        
        if (MouseButtonTransitionedDown(Interface.Mouse.LeftButtonState) &&
            PointIsInRect(LayerBounds, Interface.Mouse.Pos))
        {
            *SelectedAnimationLayer = i;
        }
        
        v2 LayerTextPos = { LayerBounds.Min.x + 6, LayerBounds.Max.y - 16};
        if (*SelectedAnimationLayer == i)
        {
            PushRenderBoundingBox2D(Interface.RenderBuffer, LayerBounds.Min, LayerBounds.Max, 1, WhiteV4);
        }
        DrawString(Interface.RenderBuffer, Layer->Name, Interface.Style.Font, LayerTextPos, WhiteV4);
    }
}

internal rect2
DrawAnimationBlock (animation_block AnimationBlock, v4 BlockColor, frame_range VisibleFrames, rect2 TimelineBounds, render_command_buffer* RenderBuffer)
{
    rect2 BlockBounds = {};
    
    r32 TimelineWidth = Rect2Width(TimelineBounds);
    
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
DrawAnimationTimeline (animation_system* AnimationSystem, animation_timeline_state* TimelineState, rect2 PanelBounds, gs_list_handle SelectedBlockHandle, ui_interface* Interface, app_state* State)
{
    gs_string Tempgs_string = PushString(State->Transient, 256);
    gs_list_handle Result = SelectedBlockHandle;
    
    // TODO(pjs): Animation Selection
    animation CurrAnimation = AnimationSystem->Animations.Values[0];
    
    rect2 LayerMenuBounds, TimelineBounds;
    RectVSplitAtDistanceFromLeft(PanelBounds, 256, &LayerMenuBounds, &TimelineBounds);
    
    // In Top To Bottom Order
    rect2 TimelineFrameBarBounds;
    rect2 TimelineBlockDisplayBounds;
    rect2 TimelineRangeBarBounds;
    RectHSplitAtDistanceFromTop(TimelineBounds, 32, &TimelineFrameBarBounds, &TimelineBounds);
    RectHSplitAtDistanceFromBottom(TimelineBounds, 24, &TimelineBlockDisplayBounds, &TimelineRangeBarBounds);
    
    DrawLayerMenu(AnimationSystem, CurrAnimation, *Interface, LayerMenuBounds, &State->SelectedAnimationLayer);
    
    frame_range AdjustedViewRange = DrawTimelineRangeBar(AnimationSystem, CurrAnimation, TimelineState, *Interface, TimelineRangeBarBounds);
    
    DrawFrameBar(AnimationSystem, *Interface, AdjustedViewRange, TimelineFrameBarBounds, State);
    
    ui_FillRect(Interface, TimelineBlockDisplayBounds, v4{.25f, .25f, .25f, 1.0f});
    
    // Animation Blocks
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState);
    gs_list_handle DragBlockHandle = {0};
    for (u32 i = 0; i < CurrAnimation.Blocks.Used; i++)
    {
        gs_list_entry<animation_block>* AnimationBlockEntry = CurrAnimation.Blocks.GetEntryAtIndex(i);
        if (EntryIsFree(AnimationBlockEntry)) { continue; }
        
        gs_list_handle CurrentBlockHandle = AnimationBlockEntry->Handle;
        animation_block AnimationBlockAt = AnimationBlockEntry->Value;
        
        // If either end is in the range, we should draw it
        b32 RangeIsVisible = (FrameIsInRange(AdjustedViewRange, AnimationBlockAt.Range.Min) ||
                              FrameIsInRange(AdjustedViewRange, AnimationBlockAt.Range.Max));
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
            rect2 BlockBounds = DrawAnimationBlock(AnimationBlockAt, BlockColor, AdjustedViewRange, TimelineBounds, Interface->RenderBuffer);
            if (PointIsInRect(BlockBounds, Interface->Mouse.Pos))
            {
                DragBlockHandle = CurrentBlockHandle;
            }
        }
    }
    
    if (MouseDownAndNotHandled && ListHandleIsValid(DragBlockHandle))
    {
        MouseDownAndNotHandled = false;
        SelectAndBeginDragAnimationBlock(DragBlockHandle, AdjustedViewRange, TimelineBounds, State);
    }
    
    // Time Slider
    if (FrameIsInRange(AdjustedViewRange, AnimationSystem->CurrentFrame))
    {
        r32 FrameAtPercentVisibleRange = FrameToPercentRange(AnimationSystem->CurrentFrame, AdjustedViewRange);
        r32 SliderX = LerpR32(FrameAtPercentVisibleRange, TimelineBounds.Min.x, TimelineBounds.Max.x);
        rect2 SliderBounds = {
            v2{ SliderX, TimelineBounds.Min.y },
            v2{ SliderX + 1, TimelineBounds.Max.y }
        };
        ui_FillRect(Interface, SliderBounds, TimeSliderColor);
    }
    
    ui_OutlineRect(Interface, TimelineRangeBarBounds, 1.f, RedV4);
    ui_OutlineRect(Interface, TimelineFrameBarBounds, 1.f, RedV4);
    ui_OutlineRect(Interface, TimelineBlockDisplayBounds, 1.f, RedV4);
    
    if (MouseDownAndNotHandled && PointIsInRect(TimelineBounds, Interface->Mouse.Pos))
    {
        DeselectCurrentAnimationBlock(State);
    }
    
    return Result;
}

internal void
DrawAnimationClipsList(rect2 PanelBounds, ui_interface* Interface, u32 SelectedAnimationLayerHandle, animation_system* AnimationSystem)
{
    ui_layout Layout = ui_CreateLayout(*Interface, PanelBounds);
    for (s32 i = 0; i < GlobalAnimationClipsCount; i++)
    {
        animation_clip Clip = GlobalAnimationClips[i];
        gs_string ClipName = MakeString(Clip.Name, Clip.NameLength);
        if (ui_LayoutListEntry(Interface, &Layout, ClipName, i))
        {
            AddAnimationBlockAtCurrentTime(i + 1, SelectedAnimationLayerHandle, AnimationSystem);
        }
    }
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Render(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    animation_timeline_state* TimelineState = (animation_timeline_state*)Panel.PanelStateMemory;
    // TODO(pjs): SelectedAnimationBlockHandle should be a property of animation_timeline_state
    // unless its used elsewhere. Audit later
    gs_list_handle SelectedBlockHandle = State->SelectedAnimationBlockHandle;
    ui_interface* Interface = &State->Interface;
    animation_system* AnimationSystem = &State->AnimationSystem;
    
    rect2 TitleBarBounds, PanelContentsBounds;
    rect2 AnimationListBounds, TimelineBounds;
    RectHSplitAtDistanceFromTop(PanelBounds, Interface->Style.RowHeight, &TitleBarBounds, &PanelContentsBounds);
    RectVSplitAtDistanceFromLeft(PanelContentsBounds, 300, &AnimationListBounds, &TimelineBounds);
    
    ui_FillRect(Interface, TitleBarBounds, Interface->Style.PanelBGColors[0]);
    ui_layout TitleBarLayout = ui_CreateLayout(*Interface, TitleBarBounds);
    ui_StartRow(&TitleBarLayout, 3);
    {
        if (ui_LayoutButton(Interface, &TitleBarLayout, MakeString("Pause")))
        {
            State->AnimationSystem.TimelineShouldAdvance = false;
        }
        
        if (ui_LayoutButton(Interface, &TitleBarLayout, MakeString("Play"), (State->AnimationSystem.TimelineShouldAdvance ? PinkV4 : BlackV4), v4{.3f, .3f, .3f, 1.0f}, TealV4))
        {
            State->AnimationSystem.TimelineShouldAdvance = true;
        }
        
        if (ui_LayoutButton(Interface, &TitleBarLayout, MakeString("Stop")))
        {
            State->AnimationSystem.TimelineShouldAdvance = false;
            State->AnimationSystem.CurrentFrame = 0;
        }
    }
    ui_EndRow(&TitleBarLayout);
    
    if (Rect2Height(TimelineBounds) > 0)
    {
        SelectedBlockHandle = DrawAnimationTimeline(AnimationSystem, TimelineState, TimelineBounds, SelectedBlockHandle, Interface, State);
        DrawAnimationClipsList(AnimationListBounds, Interface, State->SelectedAnimationLayer, &State->AnimationSystem);
    }
}

#define FOLDHAUS_PANEL_ANIMATION_TIMELINE_H
#endif // FOLDHAUS_PANEL_ANIMATION_TIMELINE_H