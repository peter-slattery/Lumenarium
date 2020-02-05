//
// File: foldhaus_animation.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
// TODO
// [] - animation blending
// [] - delete a layer
// [] - will need a way to create an empty layer
// [] - get a list of all animation procs

#ifndef FOLDHAUS_ANIMATION

#define ANIMATION_PROC(name) void name(assembly* Assembly, r32 Time)
typedef ANIMATION_PROC(animation_proc);

struct animation_block
{
    // TODO(Peter): Should we change this to frames??
    r32 StartTime;
    r32 EndTime;
    u32 AnimationProcHandle;
    
    u32 Layer;
};

#define ANIMATION_SYSTEM_LAYERS_MAX 128
#define ANIMATION_SYSTEM_BLOCKS_MAX 128
struct animation_system
{
    gs_list<animation_block> Blocks;
    
    r32 Time;
    s32 LastUpdatedFrame;
    r32 SecondsPerFrame;
    
    b32 TimelineShouldAdvance;
    
    // :Temporary
    r32 AnimationStart;
    r32 AnimationEnd;
};

#define FOLDHAUS_ANIMATION
#endif // FOLDHAUS_ANIMATION
