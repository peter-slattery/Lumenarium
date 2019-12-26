// TODO
// [] - Moving animation blocks
// [] - dragging beginning and end of time blocks
// [] - creating a timeblock with a specific animation
// [x] - play, pause, stop, 
// [] - setting the start and end of the animation system
// [] - displaying multiple layers
// [] - 

FOLDHAUS_INPUT_COMMAND_PROC(DeleteAnimationBlock)
{
    if (AnimationBlockHandleIsValid(State->SelectedAnimationBlockHandle))
    {
        RemoveAnimationBlock(State->SelectedAnimationBlockHandle, &State->AnimationSystem);
        State->SelectedAnimationBlockHandle = {0};
    }
}

internal animation_block_handle
DrawAnimationTimeline (animation_system* AnimationSystem, v2 PanelMin, v2 PanelMax, animation_block_handle SelectedBlockHandle, render_command_buffer* RenderBuffer, interface_config Interface, mouse_state Mouse)
{
    animation_block_handle Result = SelectedBlockHandle;
    
r32 AnimationPanelHeight = PanelMax.y - PanelMin.y;
            r32 AnimationPanelWidth = PanelMax.x - PanelMin.x;
            panel_result AnimationPanel = EvaluatePanel(RenderBuffer, PanelMin, PanelMax,
                                                        0, Interface);
    
    b32 MouseDownAndNotHandled = MouseButtonTransitionedDown(Mouse.LeftButtonState);
    
            for (u32 i = 0; i < AnimationSystem->BlocksCount; i++)
            {
                animation_block_entry AnimationBlockEntry = AnimationSystem->Blocks[i];
                if (AnimationBlockIsFree(AnimationBlockEntry)) { continue; }
        
        animation_block_handle CurrentBlockHandle = {};
        CurrentBlockHandle.Index = i;
        CurrentBlockHandle.Generation = AnimationBlockEntry.Generation;
        
                animation_block AnimationBlockAt = AnimationBlockEntry.Block;
                
r32 StartTimePercent = AnimationBlockAt.StartTime / AnimationSystem->AnimationEnd;
                r32 StartPosition = AnimationPanelWidth * StartTimePercent;
                
r32 EndTimePercent = AnimationBlockAt.EndTime / AnimationSystem->AnimationEnd;
                r32 EndPosition = AnimationPanelWidth * EndTimePercent;
                
                v2 Min = v2{StartPosition, 25};
                v2 Max = v2{EndPosition, 75};
        
        v4 BlockColor = BlackV4;
        if (AnimationBlockHandlesAreEqual(SelectedBlockHandle, CurrentBlockHandle))
        {
            BlockColor = PinkV4;
        }
        
                PushRenderQuad2D(RenderBuffer, Min, Max, BlockColor);
        PushRenderBoundingBox2D(RenderBuffer, Min, Max, 1, WhiteV4);
        
        if (PointIsInRange(Mouse.Pos, Min, Max)
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
    
            r32 TimePercent = AnimationSystem->Time / AnimationSystem->AnimationEnd;
            r32 SliderPosition = AnimationPanelWidth * TimePercent;
            PushRenderQuad2D(RenderBuffer, v2{SliderPosition, AnimationPanelHeight},
                     v2{SliderPosition + 1, 0}, WhiteV4);
    
    if (MouseDownAndNotHandled && PointIsInRange(Mouse.Pos, PanelMin, PanelMax))
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

internal animation_block_handle
DrawAnimationPanel (animation_system* AnimationSystem, 
v2 PanelMin, v2 PanelMax, 
animation_block_handle SelectedBlockHandle, 
render_command_buffer* RenderBuffer, 
interface_config Interface, mouse_state Mouse)
{
    animation_block_handle Result = SelectedBlockHandle;
    
r32 OptionsRowHeight = 25;
    v2 TimelineMin = PanelMin;
    v2 TimelineMax = v2{PanelMax.x, PanelMax.y - OptionsRowHeight};
    if (TimelineMax.y - TimelineMin.y > 0)
    {
        Result = DrawAnimationTimeline(AnimationSystem, 
TimelineMin, TimelineMax, 
                              SelectedBlockHandle, 
                              RenderBuffer, Interface, Mouse);
    }
    
    v2 OptionsRowMin = v2{ PanelMin.x, TimelineMax.y };
    v2 OptionsRowMax = PanelMax;
    panel_result AnimationPanel = EvaluatePanel(RenderBuffer, OptionsRowMin, OptionsRowMax,
                                                0, Interface);
    
    r32 ButtonWidth = 35;
    v2 ButtonMin = v2{0, 0};
    v2 ButtonMax = v2{35, OptionsRowHeight - 2};
    v2 ButtonAt = v2{OptionsRowMin.x + 1, OptionsRowMin.y + 1};
    
    button_result PauseResult = EvaluateButton(RenderBuffer, 
                                               ButtonAt + ButtonMin, ButtonAt + ButtonMax,
                                               MakeStringLiteral("Pause"),
                                               Interface, Mouse);
    ButtonAt.x += ButtonWidth + 2;
button_result PlayResult = EvaluateButton(RenderBuffer, 
                                               ButtonAt + ButtonMin, ButtonAt + ButtonMax,
                                               MakeStringLiteral("Play"),
                                              Interface, Mouse);
    ButtonAt.x += ButtonWidth + 2;
button_result StopResult = EvaluateButton(RenderBuffer, 
                                               ButtonAt + ButtonMin, ButtonAt + ButtonMax,
                                               MakeStringLiteral("Stop"),
                                              Interface, Mouse);
    
    if (PauseResult.Pressed)
    {
        AnimationSystem->TimelineShouldAdvance = false;
    }
    
    if (PlayResult.Pressed)
    {
        AnimationSystem->TimelineShouldAdvance = true;
    }
    
    if (StopResult.Pressed)
    {
        AnimationSystem->TimelineShouldAdvance = false;
        AnimationSystem->Time = 0;
    }
    
    return Result;
}