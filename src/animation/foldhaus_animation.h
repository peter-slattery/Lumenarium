//
// File: foldhaus_animation.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ANIMATION

#define ANIMATION_PROC(name) void name(assembly* Assembly, r32 Time)
typedef ANIMATION_PROC(animation_proc);

struct animation_block
{
    u32 StartFrame;
    u32 EndFrame;
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
    r32 StartFrame;
    r32 EndFrame;
};

internal u32
SecondsToFrames(r32 Seconds, animation_system System)
{
    u32 Result = Seconds * (1.0f / System.SecondsPerFrame);
    return Result;
}

#define FOLDHAUS_ANIMATION
#endif // FOLDHAUS_ANIMATION
