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
    handle SelectedBlockHandle;
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

internal handle
AddAnimationBlockAtCurrentTime (u32 AnimationProcHandle, u32 LayerHandle, animation_system* System)
{
    u32 NewBlockStart = System->CurrentFrame;
    u32 NewBlockEnd = NewBlockStart + SecondsToFrames(3, *System);
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
    handle AnimHandle = Animation_AddBlock(ActiveAnim, NewBlockStart, NewBlockEnd, AnimationProcHandle, LayerHandle);
    return AnimHandle;
}

FOLDHAUS_INPUT_COMMAND_PROC(DeleteAnimationBlockCommand)
{
    animation_timeline_state* PanelState = Panel_GetStateStruct(Panel, animation_timeline_state);
    
    handle SelectedBlockHandle = PanelState->SelectedBlockHandle;
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    if(SelectedBlockHandle.Index < ActiveAnim->Blocks_.Count &&
       ActiveAnim->Blocks_.Generations[SelectedBlockHandle.Index] == SelectedBlockHandle.Generation)
    {
        Animation_RemoveBlock(ActiveAnim, PanelState->SelectedBlockHandle);
        PanelState->SelectedBlockHandle = {0};
        // TODO(pjs): Introduce an animation_block_selection in this file
        // it should have a handle to the animation, block, and a HasSelection flag
        // as it is now, you kind of always have the first block selected
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
    handle BlockHandle;
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
    
    animation_block* AnimationBlock = Animation_GetBlockFromHandle(ActiveAnim, OpState->BlockHandle);
    if (!AnimationBlock)
    {
        EndCurrentOperationMode(State);
        return;
    }
    
    if (Abs(Mouse.DownPos.x - ClipInitialStartFrameXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        s32 NewStartFrame = OpState->ClipRange.Min + FrameOffset;
        if (FrameOffset < 0)
        {
            for (u32 i = 0; i < ActiveAnim->Blocks_.Count; i++)
            {
                animation_block OtherBlock = ActiveAnim->Blocks_.Values[i];
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
            for (u32 i = 0; i < ActiveAnim->Blocks_.Count; i++)
            {
                animation_block OtherBlock = ActiveAnim->Blocks_.Values[i];
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
        for (u32 i = 0; i < ActiveAnim->Blocks_.Count; i++)
        {
            animation_block OtherBlock = ActiveAnim->Blocks_.Values[i];;
            
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
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, 0 },
};

internal void
SelectAndBeginDragAnimationBlock(animation_timeline_state* TimelineState, handle BlockHandle, frame_range VisibleRange, rect2 TimelineBounds, app_state* State)
{
    TimelineState->SelectedBlockHandle = BlockHandle;
    
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    operation_mode* DragAnimationClipMode = ActivateOperationModeWithCommands(&State->Modes, DragAnimationClipCommands, UpdateDragAnimationClip);
    
    animation_block* SelectedBlock = Animation_GetBlockFromHandle(ActiveAnim, BlockHandle);
    
    drag_animation_clip_state* OpState = CreateOperationState(DragAnimationClipMode,
                                                              &State->Modes,
                                                              drag_animation_clip_state);
    OpState->TimelineBounds = TimelineBounds;
    OpState->BlockHandle = BlockHandle;
    OpState->VisibleRange = VisibleRange;
    OpState->ClipRange = SelectedBlock->Range;
}
// -------------------

FOLDHAUS_INPUT_COMMAND_PROC(AddAnimationBlockCommand)
{
    animation_timeline_state* TimelineState = Panel_GetStateStruct(Panel, animation_timeline_state);
    
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    
    frame_range Range = ActiveAnim->PlayableRange;
    u32 MouseDownFrame = GetFrameFromPointInAnimationPanel(Mouse.Pos, Panel->Bounds, Range);
    
    handle NewBlockHandle = Animation_AddBlock(ActiveAnim, MouseDownFrame, MouseDownFrame + SecondsToFrames(3, State->AnimationSystem), 4, TimelineState->SelectedAnimationLayer);
    TimelineState->SelectedBlockHandle = NewBlockHandle;
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
    
    Panel->StateMemory = StructToData(TimelineState, animation_timeline_state);
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
    if (ui_MouseClickedRect(Interface, BarBounds))
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
    
    // TODO(pjs): If mouse is on one of the border hot spots, render an off colored square to signal the region is hot
    
    return BlockBounds;
}

internal handle
DrawAnimationTimeline (animation_system* AnimationSystem, animation_timeline_state* TimelineState, rect2 PanelBounds, handle SelectedBlockHandle, ui_interface* Interface, app_state* State)
{
    gs_string Tempgs_string = PushString(State->Transient, 256);
    handle Result = SelectedBlockHandle;
    
    animation CurrAnimation = *AnimationSystem_GetActiveAnimation(AnimationSystem);
    
    rect2 LayerMenuBounds, TimelineBounds;
    RectVSplitAtDistanceFromLeft(PanelBounds, 256, &LayerMenuBounds, &TimelineBounds);
    
    // In Top To Bottom Order
    rect2 TimelineFrameBarBounds;
    rect2 TimelineBlockDisplayBounds;
    rect2 TimelineRangeBarBounds;
    RectHSplitAtDistanceFromTop(TimelineBounds, 32, &TimelineFrameBarBounds, &TimelineBounds);
    RectHSplitAtDistanceFromBottom(TimelineBounds, 24, &TimelineBlockDisplayBounds, &TimelineRangeBarBounds);
    
    DrawLayerMenu(AnimationSystem, CurrAnimation, *Interface, LayerMenuBounds, &TimelineState->SelectedAnimationLayer);
    
    frame_range AdjustedViewRange = DrawTimelineRangeBar(AnimationSystem, CurrAnimation, TimelineState, *Interface, TimelineRangeBarBounds);
    
    DrawFrameBar(AnimationSystem, *Interface, AdjustedViewRange, TimelineFrameBarBounds, State);
    
    ui_FillRect(Interface, TimelineBlockDisplayBounds, v4{.25f, .25f, .25f, 1.0f});
    
    // Animation Blocks
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState);
    handle DragBlockHandle = {0};
    for (u32 i = 0; i < CurrAnimation.Blocks_.Count; i++)
    {
        animation_block* AnimationBlockAt = CurrAnimation.Blocks_.Values + i;
        
        // If either end is in the range, we should draw it
        b32 RangeIsVisible = (FrameIsInRange(AdjustedViewRange, AnimationBlockAt->Range.Min) ||
                              FrameIsInRange(AdjustedViewRange, AnimationBlockAt->Range.Max));
        // If neither end is in the range, but the ends surround the visible range,
        // we should still draw it.
        RangeIsVisible |= (AnimationBlockAt->Range.Min <= AdjustedViewRange.Min &&
                           AnimationBlockAt->Range.Max>= AdjustedViewRange.Max);
        if (RangeIsVisible)
        {
            v4 BlockColor = BlackV4;
            if (SelectedBlockHandle.Index == i && SelectedBlockHandle.Generation == CurrAnimation.Blocks_.Generations[i])
            {
                BlockColor = PinkV4;
            }
            rect2 BlockBounds = DrawAnimationBlock(*AnimationBlockAt, BlockColor, AdjustedViewRange, TimelineBounds, Interface->RenderBuffer);
            if (PointIsInRect(BlockBounds, Interface->Mouse.Pos))
            {
                DragBlockHandle.Index = i;
                DragBlockHandle.Generation = CurrAnimation.Blocks_.Generations[i];
            }
        }
    }
    
    if (MouseDownAndNotHandled && Handle_IsValid(DragBlockHandle))
    {
        MouseDownAndNotHandled = false;
        SelectAndBeginDragAnimationBlock(TimelineState, DragBlockHandle, AdjustedViewRange, TimelineBounds, State);
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
        TimelineState->SelectedBlockHandle = {0};
    }
    
    return Result;
}

PANEL_MODAL_OVERRIDE_CALLBACK(LoadAnimationFileCallback)
{
    Assert(ReturningFrom->TypeIndex == PanelType_FileView);
    file_view_state* FileViewState = Panel_GetStateStruct(ReturningFrom, file_view_state);
    gs_file_info FileInfo = FileViewState->SelectedFile;
    
    if (FileInfo.Path.Length > 0)
    {
        gs_file AnimFile = ReadEntireFile(Context.ThreadContext.FileHandler, FileInfo.Path);
        gs_string AnimFileString = MakeString((char*)AnimFile.Data.Memory, AnimFile.Data.Size);
        animation NewAnim = AnimParser_Parse(AnimFileString, State->AnimationSystem.Storage, GlobalAnimationClipsCount, GlobalAnimationClips);
        
        u32 NewAnimIndex = AnimationArray_Push(&State->AnimationSystem.Animations, NewAnim);
        State->AnimationSystem.ActiveAnimationIndex = NewAnimIndex;
    }
}

internal void
DrawAnimationClipsList(rect2 PanelBounds, ui_interface* Interface, u32 SelectedAnimationLayerHandle, animation_system* AnimationSystem)
{
    ui_PushLayout(Interface, PanelBounds, LayoutDirection_TopDown, MakeString("AnimClips Layout"));
    for (s32 i = 0; i < GlobalAnimationClipsCount; i++)
    {
        animation_clip Clip = GlobalAnimationClips[i];
        gs_string ClipName = MakeString(Clip.Name, Clip.NameLength);
        if (ui_LayoutListButton(Interface, ClipName, i))
        {
            AddAnimationBlockAtCurrentTime(i + 1, SelectedAnimationLayerHandle, AnimationSystem);
        }
    }
    ui_PopLayout(Interface);
}

internal void
PlayBar_Render(animation_timeline_state* TimelineState, rect2 Bounds, panel* Panel, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    animation_system* AnimSystem = &State->AnimationSystem;
    ui_interface* Interface = &State->Interface;
    ui_PushLayout(Interface, Bounds, LayoutDirection_TopDown, MakeString("PlayBar Layout"));
    
    ui_FillRect(Interface, Bounds, Interface->Style.PanelBGColors[0]);
    ui_StartRow(&State->Interface, 4);
    {
        if (ui_Button(Interface, MakeString("Pause")))
        {
            AnimSystem->TimelineShouldAdvance = false;
        }
        
        if (ui_Button(Interface, MakeString("Play")))
        {
            AnimSystem->TimelineShouldAdvance = true;
        }
        
        if (ui_Button(Interface, MakeString("Stop")))
        {
            AnimSystem->TimelineShouldAdvance = false;
            AnimSystem->CurrentFrame = 0;
        }
        
        if (ui_Button(Interface, MakeString("Load")))
        {
            panel* FileBrowser = PanelSystem_PushPanel(&State->PanelSystem, PanelType_FileView, State, Context);
            Panel_PushModalOverride(Panel, FileBrowser, LoadAnimationFileCallback);
        }
    }
    ui_EndRow(&State->Interface);
    ui_PopLayout(&State->Interface);
}

internal void
FrameCount_Render(animation_timeline_state* TimelineState, rect2 Bounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_interface* Interface = &State->Interface;
    gs_string TempString = PushString(State->Transient, 256);
    frame_range VisibleFrames = TimelineState->VisibleRange;
    s32 VisibleFrameCount = VisibleFrames.Max - VisibleFrames.Min;
    
    ui_FillRect(Interface, Bounds, Interface->Style.PanelBGColors[0]);
    
    // Frame Ticks
    u32 TickCount = 10;
    for (u32 Tick = 0; Tick < TickCount; Tick++)
    {
        r32 Percent = (r32)Tick / (r32)TickCount;
        u32 Frame = PercentToFrameInRange(Percent, VisibleFrames);
        PrintF(&TempString, "%d", Frame);
        r32 FramePercent = FrameToPercentRange(Frame, VisibleFrames);
        r32 FrameX = LerpR32(FramePercent, Bounds.Min.x, Bounds.Max.x);
        v2 FrameTextPos = v2{FrameX, Bounds.Min.y + 2};
        DrawString(Interface->RenderBuffer, TempString, Interface->Style.Font, FrameTextPos, WhiteV4);
    }
    
    // Time Slider
    s32 CurrentFrame = State->AnimationSystem.CurrentFrame;
    if (FrameIsInRange(VisibleFrames, CurrentFrame))
    {
        r32 FrameAtPercentVisibleRange = FrameToPercentRange(CurrentFrame, VisibleFrames);
        r32 SliderX = LerpR32(FrameAtPercentVisibleRange, Bounds.Min.x, Bounds.Max.x);
        
        PrintF(&TempString, "%d", CurrentFrame);
        
        // space for each character + a margin on either side
        r32 SliderWidth = (8 * TempString.Length) + 8;
        r32 SliderHalfWidth = SliderWidth / 2.f;
        v2 HeadMin = v2{SliderX - SliderHalfWidth, Bounds.Min.y};
        v2 HeadMax = v2{SliderX + SliderHalfWidth, Bounds.Max.y};
        PushRenderQuad2D(Interface->RenderBuffer, HeadMin, HeadMax, TimeSliderColor);
        DrawString(Interface->RenderBuffer, TempString, Interface->Style.Font, HeadMin + v2{6, 4}, WhiteV4);
    }
    
    // Interaction
    // Mouse clicked inside frame nubmer bar -> change current frame on timeline
    if (ui_MouseClickedRect(*Interface, Bounds))
    {
        StartDragTimeMarker(Bounds, VisibleFrames, State);
    }
}

internal void
LayerList_Render(animation_timeline_state* TimelineState, rect2 Bounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_interface* Interface = &State->Interface;
    animation ActiveAnim = *AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    
    ui_FillRect(Interface, Bounds, Interface->Style.PanelBGColors[0]);
    
    v2 LayerDim = { Rect2Width(Bounds), LAYER_HEIGHT };
    rect2 LayerBounds = {0};
    LayerBounds.Min = Bounds.Min;
    LayerBounds.Max = LayerBounds.Min + LayerDim;
    for (u32 i = 0; i < ActiveAnim.Layers.Count; i++)
    {
        anim_layer* Layer = ActiveAnim.Layers.Values + i;
        
        if (ui_MouseClickedRect(*Interface, LayerBounds))
        {
            TimelineState->SelectedAnimationLayer = i;
        }
        
        v2 LayerTextPos = { LayerBounds.Min.x + 6, LayerBounds.Max.y - 16};
        if (TimelineState->SelectedAnimationLayer == i)
        {
            PushRenderBoundingBox2D(Interface->RenderBuffer, LayerBounds.Min, LayerBounds.Max, 1, WhiteV4);
        }
        DrawString(Interface->RenderBuffer, Layer->Name, Interface->Style.Font, LayerTextPos, WhiteV4);
        
        LayerBounds = Rect2TranslateY(LayerBounds, LayerDim.y);
    }
}

internal void
TimeRange_Render(animation_timeline_state* TimelineState, rect2 Bounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_interface* Interface = &State->Interface;
    frame_range ViewRange = TimelineState->VisibleRange;
    
    animation ActiveAnim = *AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    handle SelectedBlockHandle = TimelineState->SelectedBlockHandle;
    s32 CurrentFrame = State->AnimationSystem.CurrentFrame;
    
    // Animation Blocks
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState);
    handle DragBlockHandle = {0};
    for (u32 i = 0; i < ActiveAnim.Blocks_.Count; i++)
    {
        animation_block* AnimationBlockAt = ActiveAnim.Blocks_.Values + i;
        
        // If either end is in the range, we should draw it
        b32 RangeIsVisible = (FrameIsInRange(ViewRange, AnimationBlockAt->Range.Min) ||
                              FrameIsInRange(ViewRange, AnimationBlockAt->Range.Max));
        // If neither end is in the range, but the ends surround the visible range,
        // we should still draw it.
        RangeIsVisible |= (AnimationBlockAt->Range.Min <= ViewRange.Min &&
                           AnimationBlockAt->Range.Max>= ViewRange.Max);
        if (RangeIsVisible)
        {
            v4 BlockColor = BlackV4;
            if (SelectedBlockHandle.Index == i && SelectedBlockHandle.Generation == ActiveAnim.Blocks_.Generations[i])
            {
                BlockColor = PinkV4;
            }
            rect2 BlockBounds = DrawAnimationBlock(*AnimationBlockAt, BlockColor, ViewRange, Bounds, Interface->RenderBuffer);
            
            if (PointIsInRect(BlockBounds, Interface->Mouse.Pos))
            {
                DragBlockHandle.Index = i;
                DragBlockHandle.Generation = ActiveAnim.Blocks_.Generations[i];
            }
        }
    }
    
    // Time Slider
    if (FrameIsInRange(ViewRange, CurrentFrame))
    {
        r32 FrameAtPercentVisibleRange = FrameToPercentRange(CurrentFrame, ViewRange);
        r32 SliderX = LerpR32(FrameAtPercentVisibleRange, Bounds.Min.x, Bounds.Max.x);
        rect2 SliderBounds = {
            v2{ SliderX, Bounds.Min.y },
            v2{ SliderX + 1, Bounds.Max.y }
        };
        ui_FillRect(Interface, SliderBounds, TimeSliderColor);
    }
    
    // Interaction
    if (MouseDownAndNotHandled)
    {
        if (Handle_IsValid(DragBlockHandle))
        {
            MouseDownAndNotHandled = false;
            SelectAndBeginDragAnimationBlock(TimelineState, DragBlockHandle, ViewRange, Bounds, State);
        }
        else if (PointIsInRect(Bounds, Interface->Mouse.Pos))
        {
            TimelineState->SelectedBlockHandle = {0};
        }
    }
}

internal void
AnimInfoView_Render(animation_timeline_state* TimelineState, rect2 Bounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    animation_system* AnimSystem = &State->AnimationSystem;
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(AnimSystem);
    
    ui_interface* Interface = &State->Interface;
    ui_PushLayout(Interface, Bounds, LayoutDirection_TopDown, MakeString("AnimInfo Layout"));
    
    ui_FillRect(&State->Interface, Bounds, Interface->Style.PanelBGColors[0]);
    
    ui_StartRow(&State->Interface, 2);
    {
        ui_DrawString(Interface, MakeString("Active Animation"));
        if (ui_BeginDropdown(Interface, ActiveAnim->Name))
        {
            for (u32 i = 0; i < AnimSystem->Animations.Count; i++)
            {
                animation Animation = AnimSystem->Animations.Values[i];
                if (ui_Button(Interface, Animation.Name))
                {
                    AnimSystem->ActiveAnimationIndex = i;
                }
            }
        }
        ui_EndDropdown(Interface);
    }
    ui_EndRow(&State->Interface);
    ui_PopLayout(Interface);
}

internal void
SelectionInfoView_Render(animation_timeline_state* TimelineState, rect2 Bounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_FillRect(&State->Interface, Bounds, YellowV4);
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_animation_timeline);
internal void
AnimationTimeline_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    animation_timeline_state* TimelineState = Panel_GetStateStruct(Panel, animation_timeline_state);
    
    rect2 TimelineBounds, InfoBounds;
    RectVSplit(PanelBounds, 300, &InfoBounds, &TimelineBounds);
    
    rect2 AnimInfoBounds, SelectionInfoBounds;
    RectHSplitAtPercent(InfoBounds, .65f, &AnimInfoBounds, &SelectionInfoBounds);
    
    { // Timeline
        rect2 LayersPanelBounds, TimeRangePanelBounds;
        RectVSplitAtDistanceFromLeft(TimelineBounds, 200, &LayersPanelBounds, &TimeRangePanelBounds);
        
        r32 TitleBarHeight = State->Interface.Style.RowHeight;
        // These are the actual rects we will draw in
        rect2 PlayBarBounds, FrameCountBounds;
        rect2 LayersBounds, TimeRangeBounds;
        RectHSplitAtDistanceFromTop(LayersPanelBounds, TitleBarHeight, &PlayBarBounds, &LayersBounds);
        RectHSplitAtDistanceFromTop(TimeRangePanelBounds, TitleBarHeight, &FrameCountBounds, &TimeRangeBounds);
        
        PlayBar_Render(TimelineState, PlayBarBounds, Panel, RenderBuffer, State, Context);
        FrameCount_Render(TimelineState, FrameCountBounds, RenderBuffer, State, Context);
        LayerList_Render(TimelineState, LayersBounds, RenderBuffer, State, Context);
        TimeRange_Render(TimelineState, TimeRangeBounds, RenderBuffer, State, Context);
    }
    
    AnimInfoView_Render(TimelineState, AnimInfoBounds, RenderBuffer, State, Context);
    SelectionInfoView_Render(TimelineState, SelectionInfoBounds, RenderBuffer, State, Context);
}

#define FOLDHAUS_PANEL_ANIMATION_TIMELINE_H
#endif // FOLDHAUS_PANEL_ANIMATION_TIMELINE_H