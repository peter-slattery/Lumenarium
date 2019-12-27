// TODO
// [] - Moving animation blocks
// [] - dragging beginning and end of time blocks
// [] - creating a timeblock with a specific animation
// [x] - play, pause, stop, 
// [] - setting the start and end of the animation system
// [] - displaying multiple layers
// [] - 

PANEL_INIT_PROC(AnimationTimeline_Init)
{
    
}

PANEL_CLEANUP_PROC(AnimationTimeline_Cleanup)
{
    
}

internal r32
DrawFrameBar (animation_system* AnimationSystem, render_command_buffer* RenderBuffer, s32 StartFrame, s32 EndFrame, v2 PanelMin, v2 PanelMax, interface_config Interface, mouse_state Mouse)
{
    MakeStringBuffer(TempString, 256);
    
    s32 FrameCount = EndFrame - StartFrame;
    
    r32 FrameBarHeight = 24;
    v2 FrameBarMin = v2{PanelMin.x, PanelMax.y - FrameBarHeight};
    v2 FrameBarMax = PanelMax;
    
    PushRenderQuad2D(RenderBuffer, FrameBarMin, FrameBarMax, v4{.16f, .16f, .16f, 1.f});
    
    // Mouse clicked inside frame nubmer bar -> change current frame on timeline
    if (MouseButtonHeldDown(Mouse.LeftButtonState)
        && PointIsInRange(Mouse.DownPos, FrameBarMin, FrameBarMax))
    {
        r32 MouseX = Mouse.DownPos.x;
        r32 StartFrameTime = (r32)StartFrame * AnimationSystem->SecondsPerFrame;
        r32 EndFrameTime = (r32)EndFrame * AnimationSystem->SecondsPerFrame;
        r32 MouseTime = GSRemap(MouseX, FrameBarMin.x, FrameBarMax.x, StartFrameTime, EndFrameTime);
        AnimationSystem->Time = MouseTime;
    }
    
    // Frame Ticks
    for (s32 f = 0; f < FrameCount; f += 10)
    {
        s32 Frame = StartFrame + f;
        PrintF(&TempString, "%d", Frame);
        
        r32 FramePercent = (r32)f / (r32)FrameCount;
        r32 FrameX = GSLerp(PanelMin.x, PanelMax.x, FramePercent);
        v2 FrameTextPos = v2{FrameX, FrameBarMin.y + 2};
        DrawString(RenderBuffer, TempString, Interface.Font, FrameTextPos, WhiteV4);
        
        // Frame Vertical Slices
        v2 LineTop = v2{FrameX, FrameBarMin.y};
        v2 LineBottom = v2{FrameX + 1, PanelMin.y};
        PushRenderQuad2D(RenderBuffer, LineTop, LineBottom, v4{.16f, .16f, .16f, 1.f});
    }
    
    return FrameBarMin.y;
}

internal rect
DrawAnimationBlock (animation_block AnimationBlock, v4 BlockColor, r32 SecondsPerFrame, s32 FrameCount, s32 StartFrame, v2 TimelineMin, v2 TimelineMax, render_command_buffer* RenderBuffer)
{
    rect BlockBounds = {};
    
    r32 TimelineWidth = TimelineMax.x - TimelineMin.x;
    
    s32 BlockStartFrame = AnimationBlock.StartTime / SecondsPerFrame;
    r32 StartFramePercent = (r32)(BlockStartFrame - StartFrame) / (r32)FrameCount;
    r32 StartPosition = TimelineWidth * StartFramePercent;
    
    s32 BlockEndFrame = AnimationBlock.EndTime / SecondsPerFrame;
    r32 EndFramePercent = (r32)(BlockEndFrame - StartFrame) / (r32)FrameCount;
    r32 EndPosition = TimelineWidth * EndFramePercent;
    
    BlockBounds.Min = TimelineMin + v2{StartPosition, 25};
    BlockBounds.Max = TimelineMin + v2{EndPosition, 75};
    
    PushRenderQuad2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, BlockColor);
    PushRenderBoundingBox2D(RenderBuffer, BlockBounds.Min, BlockBounds.Max, 1, WhiteV4);
    
    return BlockBounds;
}

internal animation_block_handle
DrawAnimationTimeline (animation_system* AnimationSystem, s32 StartFrame, s32 EndFrame, v2 PanelMin, v2 PanelMax, animation_block_handle SelectedBlockHandle, render_command_buffer* RenderBuffer, app_state* State, mouse_state Mouse)
{
    string TempString = MakeString(PushArray(&State->Transient, char, 256), 256);
    s32 FrameCount = EndFrame - StartFrame;
    
    animation_block_handle Result = SelectedBlockHandle;
    
    r32 AnimationPanelHeight = PanelMax.y - PanelMin.y;
    r32 AnimationPanelWidth = PanelMax.x - PanelMin.x;
    
    {
        s32 FirstPlayableFrame = (AnimationSystem->AnimationStart / AnimationSystem->SecondsPerFrame);
        s32 LastPlayableFrame = (AnimationSystem->AnimationEnd / AnimationSystem->SecondsPerFrame);
        
        r32 FirstPlayablePercentX = ((r32)(FirstPlayableFrame - StartFrame) / (r32)FrameCount);
        r32 LastPlayablePercentX = ((r32)(LastPlayableFrame - StartFrame) / (r32)FrameCount);
        
        v2 PlayableMin = v2{(FirstPlayablePercentX * AnimationPanelWidth) + PanelMin.x, PanelMin.y };
        v2 PlayableMax = v2{(LastPlayablePercentX * AnimationPanelWidth) + PanelMin.x, PanelMax.y };
        
        PushRenderQuad2D(RenderBuffer, PanelMin, PanelMax, v4{.16f, .16f, .16f, 1.f});
        PushRenderQuad2D(RenderBuffer, PlayableMin, PlayableMax, v4{.22f, .22f, .22f, 1.f});
    }
    
    r32 FrameBarBottom = DrawFrameBar(AnimationSystem, RenderBuffer, StartFrame, EndFrame, PanelMin, PanelMax, State->Interface, Mouse);
    
    // Animation Blocks
    v2 TimelineMin = PanelMin;
    v2 TimelineMax = v2{PanelMax.x, FrameBarBottom};
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Mouse.LeftButtonState);
    for (u32 i = 0; i < AnimationSystem->BlocksCount; i++)
    {
        animation_block_entry AnimationBlockEntry = AnimationSystem->Blocks[i];
        if (AnimationBlockIsFree(AnimationBlockEntry)) { continue; }
        
        animation_block_handle CurrentBlockHandle = {};
        CurrentBlockHandle.Index = i;
        CurrentBlockHandle.Generation = AnimationBlockEntry.Generation;
        
        animation_block AnimationBlockAt = AnimationBlockEntry.Block;
        
        v4 BlockColor = BlackV4;
        if (AnimationBlockHandlesAreEqual(SelectedBlockHandle, CurrentBlockHandle))
        {
            BlockColor = PinkV4;
        }
        
        rect BlockBounds = DrawAnimationBlock(AnimationBlockAt, BlockColor, AnimationSystem->SecondsPerFrame, FrameCount, StartFrame, TimelineMin, TimelineMax, RenderBuffer);
        
        if (PointIsInRange(Mouse.Pos, BlockBounds.Min, BlockBounds.Max)
            && MouseButtonTransitionedDown(Mouse.LeftButtonState))
        {
            MouseDownAndNotHandled = false;
            if (AnimationBlockHandlesAreEqual(SelectedBlockHandle, CurrentBlockHandle))
            {
                // If the block is already selected, deselect it.
                Result = {0};
            }
            else
            {
                Result = CurrentBlockHandle;
            }
        }
    }
    
    // Time Slider
    s32 SliderFrame = AnimationSystem->Time / AnimationSystem->SecondsPerFrame;
    r32 TimePercent = (r32)(SliderFrame - StartFrame) / (r32)FrameCount;
    r32 SliderX = PanelMin.x + (AnimationPanelWidth * TimePercent);
    v2 SliderMin = v2{SliderX, PanelMin.y};
    v2 SliderMax = v2{SliderX + 1, PanelMax.y - 25};
    v4 TimeSliderColor = v4{.36f, .52f, .78f, 1.f};
    
    PushRenderQuad2D(RenderBuffer, SliderMin, SliderMax, TimeSliderColor);
    
    r32 SliderHalfWidth = 10;
    v2 HeadMin = v2{SliderX - SliderHalfWidth, SliderMax.y};
    v2 HeadMax = v2{SliderX + SliderHalfWidth, PanelMax.y};
    PushRenderQuad2D(RenderBuffer, HeadMin, HeadMax, TimeSliderColor);
    
    PrintF(&TempString, "%d", SliderFrame);
    DrawString(RenderBuffer, TempString, State->Interface.Font, HeadMin + v2{4, 4}, WhiteV4);
    
    if (MouseDownAndNotHandled && PointIsInRange(Mouse.Pos, TimelineMin, TimelineMax))
    {
        r32 MouseDownPositionPercent = (Mouse.Pos.x - PanelMin.x) / AnimationPanelWidth;
        r32 NewBlockTimeStart = MouseDownPositionPercent * AnimationSystem->AnimationEnd;
#define NEW_BLOCK_DURATION 1
        r32 NewBlockTimeEnd = NewBlockTimeStart + NEW_BLOCK_DURATION;
        
        animation_block Block = {0};
        Block.StartTime = NewBlockTimeStart;
        Block.EndTime = NewBlockTimeEnd;
        Block.Proc = TestPatternThree;
        
        animation_block_handle NewBlockHandle = AddAnimationBlock(Block, AnimationSystem);
        Result = NewBlockHandle;
    }
    return Result;
}

PANEL_RENDER_PROC(AnimationTimeline_Render)
{
    animation_block_handle SelectedBlockHandle = State->SelectedAnimationBlockHandle;
    
    r32 OptionsRowHeight = 25;
    v2 TimelineMin = PanelMin;
    v2 TimelineMax = v2{PanelMax.x, PanelMax.y - OptionsRowHeight};
    if (TimelineMax.y - TimelineMin.y > 0)
    {
        s32 FrameStart = (s32)(State->AnimationSystem.AnimationStart / State->AnimationSystem.SecondsPerFrame);
        s32 FrameEnd = (s32)(State->AnimationSystem.AnimationEnd / State->AnimationSystem.SecondsPerFrame);
        SelectedBlockHandle = DrawAnimationTimeline(&State->AnimationSystem, 
                                                    FrameStart - 20, FrameEnd + 20,
                                                    TimelineMin, TimelineMax, 
                                                    SelectedBlockHandle, 
                                                    RenderBuffer, State, Mouse);
    }
    
    v2 OptionsRowMin = v2{ PanelMin.x, TimelineMax.y };
    v2 OptionsRowMax = PanelMax;
    panel_result AnimationPanel = EvaluatePanel(RenderBuffer, OptionsRowMin, OptionsRowMax,
                                                0, State->Interface);
    
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
    
    State->SelectedAnimationBlockHandle = SelectedBlockHandle;
}
