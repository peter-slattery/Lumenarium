//
// File: foldhaus_animation.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ANIMATION

#define ANIMATION_PROC(name) void name(assembly* Assembly, r32 Time)
typedef ANIMATION_PROC(animation_proc);

struct frame_range
{
    u32 Min;
    u32 Max;
};

struct animation_block
{
    frame_range Range;
    u32 AnimationProcHandle;
    u32 Layer;
};

#define ANIMATION_SYSTEM_LAYERS_MAX 128
#define ANIMATION_SYSTEM_BLOCKS_MAX 128
struct animation_system
{
    gs_list<animation_block> Blocks;
    
    // NOTE(Peter): The frame currently being displayed/processed. you
    // can see which frame you're on by looking at the time slider on the timeline
    // panel
    u32 CurrentFrame;
    u32 LastUpdatedFrame;
    r32 SecondsPerFrame;
    
    b32 TimelineShouldAdvance;
    
    // Timeline
    frame_range PlayableRange;
};

internal u32
SecondsToFrames(r32 Seconds, animation_system System)
{
    u32 Result = Seconds * (1.0f / System.SecondsPerFrame);
    return Result;
}

inline b32
FrameIsInRange(u32 Frame, frame_range Range)
{
    b32 Result = (Frame >= Range.Min) && (Frame <= Range.Max);
    return Result;
}

internal u32
GetFrameCount(frame_range Range)
{
    u32 Result = Range.Max - Range.Min;
    return Result;
}

internal r32
FrameToPercentRange(s32 Frame, frame_range Range)
{
    r32 Result = (r32)(Frame - Range.Min);
    Result = Result / GetFrameCount(Range);
    return Result;
}

internal u32
PercentToFrameInRange(r32 Percent, frame_range Range)
{
    u32 Result = Range.Min + (u32)(Percent * GetFrameCount(Range));
    return Result;
}

internal u32 
ClampFrameToRange(u32 Frame, frame_range Range)
{
    u32 Result = Frame;
    if (Result < Range.Min)
    {
        Result = Range.Min;
    }
    else if (Result > Range.Max)
    {
        Result = Range.Max;
    }
    return Result;
}

#define FOLDHAUS_ANIMATION
#endif // FOLDHAUS_ANIMATION