//
// File: foldhaus_panel_animation_timeline.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_ANIMATION_TIMELINE_H

// TODO
// [x] - Moving animation blocks
// [x] - dragging beginning and end of time blocks
// [] - creating a timeblock with a specific animation
// [x] - play, pause, stop, 
// [] - setting the start and end of the animation system
// [] - displaying multiple layers

inline r32
GetTimeFromPointInAnimationPanel(v2 Point, rect PanelBounds, s32 StartFrame, s32 EndFrame, r32 SecondsPerFrame)
{
    r32 StartFrameTime = (r32)StartFrame * SecondsPerFrame;
    r32 EndFrameTime = (r32)EndFrame * SecondsPerFrame;
    r32 TimeAtPoint = GSRemap(Point.x, PanelBounds.Min.x, PanelBounds.Max.x, StartFrameTime, EndFrameTime);
    return TimeAtPoint;
}

inline s32
GetFrameFromPointInAnimationPanel (v2 Point, rect PanelBounds, s32 StartFrame, s32 EndFrame, r32 SecondsPerFrame)
{
    r32 TimeAtPoint = GetTimeFromPointInAnimationPanel(Point, PanelBounds, StartFrame, EndFrame, SecondsPerFrame);
    s32 FrameAtPoint = (s32)(TimeAtPoint * SecondsPerFrame);
    return FrameAtPoint;
}

inline s32
GetXPositionFromTimeInAnimationPanel (r32 Time, rect PanelBounds, s32 StartFrame, s32 EndFrame, r32 SecondsPerFrame)
{
    r32 StartFrameTime = (r32)StartFrame * SecondsPerFrame;
    r32 EndFrameTime = (r32)EndFrame * SecondsPerFrame;
    s32 XPositionAtTime = GSRemap(Time, StartFrameTime, EndFrameTime, PanelBounds.Min.x, PanelBounds.Max.x);
    return XPositionAtTime;
}

internal void
AddAnimationBlock(r32 StartTime, r32 EndTime, u32 AnimationProcHandle, animation_system* AnimationSystem)
{
    animation_block NewBlock = {0};
    NewBlock.StartTime = StartTime;
    NewBlock.EndTime = EndTime;
    NewBlock.AnimationProcHandle = AnimationProcHandle;
    AnimationSystem->Blocks.PushElementOnList(NewBlock);
}

#define NEW_ANIMATION_BLOCK_DURATION 3
internal void
AddAnimationBlockAtCurrentTime (u32 AnimationProcHandle, animation_system* System)
{
    r32 CurrentTime = System->Time;
    AddAnimationBlock(CurrentTime, CurrentTime + NEW_ANIMATION_BLOCK_DURATION, AnimationProcHandle, System);
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
    DeleteAnimationBlock(State->SelectedAnimationBlockHandle, State);
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
    r32 TimeAtMouseX = GetTimeFromPointInAnimationPanel(Mouse.Pos, OpState->TimelineBounds, OpState->StartFrame, OpState->EndFrame, State->AnimationSystem.SecondsPerFrame);
    State->AnimationSystem.Time = TimeAtMouseX;
}

FOLDHAUS_INPUT_COMMAND_PROC(EndDragTimeMarker)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command DragTimeMarkerCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndDragTimeMarker },
};

internal void
StartDragTimeMarker(rect TimelineBounds, s32 PanelStartFrame, s32 PanelEndFrame, app_state* State)
{
    operation_mode* DragTimeMarkerMode = ActivateOperationModeWithCommands(&State->Modes, DragTimeMarkerCommands, UpdateDragTimeMarker);
    
    drag_time_marker_operation_state* OpState = CreateOperationState(DragTimeMarkerMode,
                                                                     &State->Modes,
                                                                     drag_time_marker_operation_state);
    OpState->StartFrame = PanelStartFrame;
    OpState->EndFrame = PanelEndFrame ;
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
    r32 AnimationPanel_StartFrame;
    r32 AnimationPanel_EndFrame;
    r32 SelectedClip_InitialStartTime;
    r32 SelectedClip_InitialEndTime;
};

OPERATION_RENDER_PROC(UpdateDragAnimationClip)
{
    drag_animation_clip_state* OpState = (drag_animation_clip_state*)Operation.OpStateMemory;
    
    s32 ClipInitialStartTimeXPosition = GetXPositionFromTimeInAnimationPanel(OpState->SelectedClip_InitialStartTime, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame, State->AnimationSystem.SecondsPerFrame);
    s32 ClipInitialEndTimeXPosition = GetXPositionFromTimeInAnimationPanel(OpState->SelectedClip_InitialEndTime, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame, State->AnimationSystem.SecondsPerFrame);
    
    r32 TimeAtMouseDownX = GetTimeFromPointInAnimationPanel(Mouse.DownPos, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame, State->AnimationSystem.SecondsPerFrame);
    r32 TimeAtMouseX = GetTimeFromPointInAnimationPanel(Mouse.Pos, OpState->TimelineBounds, OpState->AnimationPanel_StartFrame, OpState->AnimationPanel_EndFrame, State->AnimationSystem.SecondsPerFrame);
    r32 TimeOffset = TimeAtMouseX - TimeAtMouseDownX;
    
    animation_block* AnimationBlock = State->AnimationSystem.Blocks.GetElementWithHandle(State->SelectedAnimationBlockHandle);
    
    if (GSAbs(Mouse.DownPos.x - ClipInitialStartTimeXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        AnimationBlock->StartTime = OpState->SelectedClip_InitialStartTime + TimeOffset;
    }
    else if (GSAbs(Mouse.DownPos.x - ClipInitialEndTimeXPosition) < CLICK_ANIMATION_BLOCK_EDGE_MAX_SCREEN_DISTANCE)
    {
        AnimationBlock->EndTime = OpState->SelectedClip_InitialEndTime + TimeOffset;
    }
    else
    {
        AnimationBlock->StartTime = OpState->SelectedClip_InitialStartTime + TimeOffset;
        AnimationBlock->EndTime = OpState->SelectedClip_InitialEndTime + TimeOffset;
    }
}

input_command DragAnimationClipCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndCurrentOperationMode },
};

internal void
SelectAndBeginDragAnimationBlock(gs_list_handle BlockHandle, s32 PanelStartFrame, s32 PanelEndFrame, rect TimelineBounds, app_state* State)
{
    SelectAnimationBlock(BlockHandle, State);
    
    operation_mode* DragAnimationClipMode = ActivateOperationModeWithCommands(&State->Modes, DragAnimationClipCommands, UpdateDragAnimationClip);
    
    drag_animation_clip_state* OpState = CreateOperationState(DragAnimationClipMode,
                                                              &State->Modes,
                                                              drag_animation_clip_state);
    OpState->TimelineBounds = TimelineBounds;
    OpState->AnimationPanel_StartFrame = PanelStartFrame;
    OpState->AnimationPanel_EndFrame = PanelEndFrame ;
    
    animation_block* SelectedBlock = State->AnimationSystem.Blocks.GetElementWithHandle(BlockHandle);
    OpState->SelectedClip_InitialStartTime = SelectedBlock->StartTime;
    OpState->SelectedClip_InitialEndTime = SelectedBlock->EndTime;
}

// -------------------

FOLDHAUS_INPUT_COMMAND_PROC(AddAnimationBlockCommand)
{
    panel_and_bounds ActivePanel = GetPanelContainingPoint(Mouse.Pos, &State->PanelSystem, State->WindowBounds);
    r32 MouseDownPositionPercent = (Mouse.Pos.x - ActivePanel.Bounds.Min.x) / Width(ActivePanel.Bounds);
    r32 NewBlockTimeStart = MouseDownPositionPercent * State->AnimationSystem.AnimationEnd;
#define NEW_BLOCK_DURATION 1
    r32 NewBlockTimeEnd = NewBlockTimeStart + NEW_BLOCK_DURATION;
    
    animation_block Block = {0};
    Block.StartTime = NewBlockTimeStart;
    Block.EndTime = NewBlockTimeEnd;
    Block.AnimationProcHandle = 4;
    
    gs_list_handle NewBlockHandle = State->AnimationSystem.Blocks.PushElementOnList(Block);
    SelectAnimationBlock(NewBlockHandle, State);
}

input_command AnimationTimeline_Commands[] = {
    { KeyCode_X, KeyCode_Invalid, Command_Began, DeleteAnimationBlockCommand },
    { KeyCode_A, KeyCode_Invalid, Command_Began, AddAnimationBlockCommand },
};


GSMetaTag(panel_init);
internal void
AnimationTimeline_Init(panel* Panel, app_state* State)
{
    
}

GSMetaTag(panel_cleanup);
internal void
AnimationTimeline_Cleanup(panel* Panel, app_state* State)
{
    
}

internal r32
DrawFrameBar (animation_system* AnimationSystem, render_command_buffer* RenderBuffer, s32 StartFrame, s32 EndFrame, rect PanelBounds, mouse_state Mouse, app_state* State)
{
    MakeStringBuffer(TempString, 256);
    
    s32 FrameCount = EndFrame - StartFrame;
    
    r32 FrameBarHeight = 24;
    v2 FrameBarMin = v2{PanelBounds.Min.x, PanelBounds.Max.y - FrameBarHeight};
    v2 FrameBarMax = PanelBounds.Max;
    
    PushRenderQuad2D(RenderBuffer, FrameBarMin, FrameBarMax, v4{.16f, .16f, .16f, 1.f});
    
    // Mouse clicked inside frame nubmer bar -> change current frame on timeline
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState)
        && PointIsInRange(Mouse.DownPos, FrameBarMin, FrameBarMax))
    {
        StartDragTimeMarker(rect{FrameBarMin, FrameBarMax}, StartFrame, EndFrame, State);
    }
    
    // Frame Ticks
    for (s32 f = 0; f < FrameCount; f += 10)
    {
        s32 Frame = StartFrame + f;
        PrintF(&TempString, "%d", Frame);
        
        r32 FramePercent = (r32)f / (r32)FrameCount;
        r32 FrameX = GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, FramePercent);
        v2 FrameTextPos = v2{FrameX, FrameBarMin.y + 2};
        DrawString(RenderBuffer, TempString, State->Interface.Font, FrameTextPos, WhiteV4);
        
        // Frame Vertical Slices
        v2 LineTop = v2{FrameX, FrameBarMin.y};
        v2 LineBottom = v2{FrameX + 1, PanelBounds.Min.y};
        PushRenderQuad2D(RenderBuffer, LineTop, LineBottom, v4{.16f, .16f, .16f, 1.f});
    }
    
    return FrameBarMin.y;
}

internal rect
DrawAnimationBlock (animation_block AnimationBlock, v4 BlockColor, r32 SecondsPerFrame, s32 FrameCount, s32 StartFrame, rect TimelineBounds, render_command_buffer* RenderBuffer)
{
    rect BlockBounds = {};
    
    r32 TimelineWidth = Width(TimelineBounds);
    
    s32 BlockStartFrame = AnimationBlock.StartTime / SecondsPerFrame;
    r32 StartFramePercent = (r32)(BlockStartFrame - StartFrame) / (r32)FrameCount;
    r32 StartPosition = TimelineWidth * StartFramePercent;
    
    s32 BlockEndFrame = AnimationBlock.EndTime / SecondsPerFrame;
    r32 EndFramePercent = (r32)(BlockEndFrame - StartFrame) / (r32)FrameCount;
    r32 EndPosition = TimelineWidth * EndFramePercent;
    
    BlockBounds.Min = TimelineBounds.Min + v2{StartPosition, 25};
    BlockBounds.Max = TimelineBounds.Min + v2{EndPosition, 75};
    
    PushRenderQuad2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, BlockColor);
    PushRenderBoundingBox2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, 1, WhiteV4);
    
    return BlockBounds;
}

internal gs_list_handle
DrawAnimationTimeline (animation_system* AnimationSystem, s32 StartFrame, s32 EndFrame, rect PanelBounds, gs_list_handle SelectedBlockHandle, render_command_buffer* RenderBuffer, app_state* State, mouse_state Mouse)
{
    string TempString = MakeString(PushArray(&State->Transient, char, 256), 256);
    s32 FrameCount = EndFrame - StartFrame;
    
    gs_list_handle Result = SelectedBlockHandle;
    
    r32 AnimationPanelHeight = PanelBounds.Max.y - PanelBounds.Min.y;
    r32 AnimationPanelWidth = PanelBounds.Max.x - PanelBounds.Min.x;
    
    {
        s32 FirstPlayableFrame = (AnimationSystem->AnimationStart / AnimationSystem->SecondsPerFrame);
        s32 LastPlayableFrame = (AnimationSystem->AnimationEnd / AnimationSystem->SecondsPerFrame);
        
        r32 FirstPlayablePercentX = ((r32)(FirstPlayableFrame - StartFrame) / (r32)FrameCount);
        r32 LastPlayablePercentX = ((r32)(LastPlayableFrame - StartFrame) / (r32)FrameCount);
        
        v2 PlayableMin = v2{(FirstPlayablePercentX * AnimationPanelWidth) + PanelBounds.Min.x, PanelBounds.Min.y };
        v2 PlayableMax = v2{(LastPlayablePercentX * AnimationPanelWidth) + PanelBounds.Min.x, PanelBounds.Max.y };
        
        PushRenderQuad2D(RenderBuffer, PanelBounds.Min, PanelBounds.Max, v4{.16f, .16f, .16f, 1.f});
        PushRenderQuad2D(RenderBuffer, PlayableMin, PlayableMax, v4{.22f, .22f, .22f, 1.f});
    }
    
    r32 FrameBarBottom = DrawFrameBar(AnimationSystem, RenderBuffer, StartFrame, EndFrame, PanelBounds, Mouse, State);
    
    // Animation Blocks
    rect TimelineBounds = rect{ PanelBounds.Min, v2{PanelBounds.Max.x, FrameBarBottom} };
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Mouse.LeftButtonState);
    for (u32 i = 0; i < AnimationSystem->Blocks.Used; i++)
    {
        gs_list_entry<animation_block>* AnimationBlockEntry = AnimationSystem->Blocks.GetEntryAtIndex(i);
        if (AnimationBlockEntry->Free.NextFreeEntry != 0) { continue; }
        
        gs_list_handle CurrentBlockHandle = AnimationBlockEntry->Handle;
        animation_block AnimationBlockAt = AnimationBlockEntry->Value;
        
        v4 BlockColor = BlackV4;
        if (GSListHandlesAreEqual(SelectedBlockHandle, CurrentBlockHandle))
        {
            BlockColor = PinkV4;
        }
        
        rect BlockBounds = DrawAnimationBlock(AnimationBlockAt, BlockColor, AnimationSystem->SecondsPerFrame, FrameCount, StartFrame, TimelineBounds, RenderBuffer);
        
        if (PointIsInRange(Mouse.Pos, BlockBounds.Min, BlockBounds.Max)
            && MouseButtonTransitionedDown(Mouse.LeftButtonState))
        {
            MouseDownAndNotHandled = false;
            SelectAndBeginDragAnimationBlock(CurrentBlockHandle, StartFrame, EndFrame, TimelineBounds, State);
        }
    }
    
    // Time Slider
    s32 SliderFrame = AnimationSystem->Time / AnimationSystem->SecondsPerFrame;
    r32 TimePercent = (r32)(SliderFrame - StartFrame) / (r32)FrameCount;
    r32 SliderX = PanelBounds.Min.x + (AnimationPanelWidth * TimePercent);
    v2 SliderMin = v2{SliderX, PanelBounds.Min.y};
    v2 SliderMax = v2{SliderX + 1, PanelBounds.Max.y - 25};
    v4 TimeSliderColor = v4{.36f, .52f, .78f, 1.f};
    
    PushRenderQuad2D(RenderBuffer, SliderMin, SliderMax, TimeSliderColor);
    
    r32 SliderHalfWidth = 10;
    v2 HeadMin = v2{SliderX - SliderHalfWidth, SliderMax.y};
    v2 HeadMax = v2{SliderX + SliderHalfWidth, PanelBounds.Max.y};
    PushRenderQuad2D(RenderBuffer, HeadMin, HeadMax, TimeSliderColor);
    
    PrintF(&TempString, "%d", SliderFrame);
    DrawString(RenderBuffer, TempString, State->Interface.Font, HeadMin + v2{4, 4}, WhiteV4);
    
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
internal void
AnimationTimeline_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
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
        DrawAnimationClipsList(AnimationClipListBounds, Mouse, RenderBuffer, State);
        
        s32 FrameStart = (s32)(State->AnimationSystem.AnimationStart / State->AnimationSystem.SecondsPerFrame);
        s32 FrameEnd = (s32)(State->AnimationSystem.AnimationEnd / State->AnimationSystem.SecondsPerFrame);
        SelectedBlockHandle = DrawAnimationTimeline(&State->AnimationSystem, 
                                                    FrameStart - 20, FrameEnd + 20,
                                                    TimelineBounds,
                                                    SelectedBlockHandle, 
                                                    RenderBuffer, State, Mouse);
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
        State->AnimationSystem.Time = 0;
    }
}

#define FOLDHAUS_PANEL_ANIMATION_TIMELINE_H
#endif // FOLDHAUS_PANEL_ANIMATION_TIMELINE_H