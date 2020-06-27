//
// File: foldhaus_animation.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ANIMATION

#define ANIMATION_PROC(name) void name(led_buffer* Assembly, r32 Time)
typedef ANIMATION_PROC(animation_proc);

struct frame_range
{
    s32 Min;
    s32 Max;
};

struct animation_block
{
    frame_range Range;
    u32 AnimationProcHandle;
    u32 Layer;
};

enum blend_mode
{
    BlendMode_Overwrite,
    BlendMode_Add,
    BlendMode_Multiply,
    BlendMode_Count,
};

struct anim_layer
{
    string Name;
    blend_mode BlendMode;
};

#define ANIMATION_SYSTEM_LAYERS_MAX 128
#define ANIMATION_SYSTEM_BLOCKS_MAX 128
struct animation_system
{
    memory_arena* Storage;
    
    gs_list<animation_block> Blocks;
    anim_layer* Layers;
    u32 LayersCount;
    u32 LayersMax;
    
    // NOTE(Peter): The frame currently being displayed/processed. you
    // can see which frame you're on by looking at the time slider on the timeline
    // panel
    s32 CurrentFrame;
    s32 LastUpdatedFrame;
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
FrameIsInRange(s32 Frame, frame_range Range)
{
    b32 Result = (Frame >= Range.Min) && (Frame <= Range.Max);
    return Result;
}

internal u32
GetFrameCount(frame_range Range)
{
    u32 Result = (u32)GSMax(0, Range.Max - Range.Min);
    return Result;
}

internal r32
FrameToPercentRange(s32 Frame, frame_range Range)
{
    r32 Result = (r32)(Frame - Range.Min);
    Result = Result / GetFrameCount(Range);
    return Result;
}

internal s32
PercentToFrameInRange(r32 Percent, frame_range Range)
{
    s32 Result = Range.Min + (s32)(Percent * GetFrameCount(Range));
    return Result;
}

internal s32
ClampFrameToRange(s32 Frame, frame_range Range)
{
    s32 Result = Frame;
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

// Blocks

internal gs_list_handle
AddAnimationBlock(u32 StartFrame, s32 EndFrame, u32 AnimationProcHandle, u32 Layer, animation_system* AnimationSystem)
{
    gs_list_handle Result = {0};
    animation_block NewBlock = {0};
    NewBlock.Range.Min = StartFrame;
    NewBlock.Range.Max = EndFrame;
    NewBlock.AnimationProcHandle = AnimationProcHandle;
    NewBlock.Layer = Layer;
    Result = AnimationSystem->Blocks.PushElementOnList(NewBlock);
    return Result;
}

internal void
RemoveAnimationBlock(gs_list_handle AnimationBlockHandle, animation_system* AnimationSystem)
{
    Assert(ListHandleIsValid(AnimationBlockHandle));
    AnimationSystem->Blocks.FreeElementWithHandle(AnimationBlockHandle);
}

// Layers
internal u32
AddLayer (string Name, animation_system* AnimationSystem, blend_mode BlendMode = BlendMode_Overwrite)
{
    // NOTE(Peter): If this assert fires its time to make the layer buffer system
    // resizable.
    Assert(AnimationSystem->LayersCount < AnimationSystem->LayersMax);
    
    u32 Result = 0;
    Result = AnimationSystem->LayersCount++;
    anim_layer* NewLayer = AnimationSystem->Layers + Result;
    *NewLayer = {0};
    NewLayer->Name = MakeString(PushArray(AnimationSystem->Storage, char, Name.Length), Name.Length);
    CopyStringTo(Name, &NewLayer->Name);
    NewLayer->BlendMode = BlendMode;
    return Result;
}

internal void
RemoveLayer (u32 LayerIndex, animation_system* AnimationSystem)
{
    Assert(LayerIndex < AnimationSystem->LayersMax);
    Assert(LayerIndex < AnimationSystem->LayersCount);
    for (u32 i = LayerIndex; i < AnimationSystem->LayersCount - 1; i++)
    {
        AnimationSystem->Layers[i] = AnimationSystem->Layers[i + 1];
    }
    for (u32 i = AnimationSystem->Blocks.Used -= 1; i >= 0; i--)
    {
        gs_list_entry<animation_block>* Entry = AnimationSystem->Blocks.GetEntryAtIndex(i);
        if (EntryIsFree(Entry)) { continue; }
        animation_block* Block = &Entry->Value;
        if (Block->Layer > LayerIndex)
        {
            Block->Layer -= 1;
        }
        else if (Block->Layer == LayerIndex)
        {
            AnimationSystem->Blocks.FreeElementAtIndex(i);
        }
    }
    AnimationSystem->LayersCount -= 1;
}
#define FOLDHAUS_ANIMATION
#endif // FOLDHAUS_ANIMATION