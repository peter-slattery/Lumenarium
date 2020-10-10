//
// File: foldhaus_animation.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ANIMATION

#define ANIMATION_PROC(name) void name(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient)
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
    gs_string Name;
    blend_mode BlendMode;
};

struct anim_layer_array
{
    anim_layer* Values;
    u32 Count;
    u32 CountMax;
};

struct animation
{
    anim_layer_array Layers;
    gs_list<animation_block> Blocks;
    
    frame_range PlayableRange;
};

struct animation_array
{
    animation* Values;
    u32 Count;
    u32 CountMax;
};

struct animation_frame
{
    // NOTE(pjs): These are all parallel arrays of equal length
    animation_block* Blocks;
    b8*              BlocksFilled;
    
    u32 BlocksCountMax;
    u32 BlocksCount;
};

#define ANIMATION_SYSTEM_LAYERS_MAX 128
#define ANIMATION_SYSTEM_BLOCKS_MAX 128
struct animation_system
{
    gs_memory_arena* Storage;
    animation_array Animations;
    
    frame_range PlayableRange_;
    gs_list<animation_block> Blocks_;
    
    // NOTE(Peter): The frame currently being displayed/processed. you
    // can see which frame you're on by looking at the time slider on the timeline
    // panel
    s32 CurrentFrame;
    s32 LastUpdatedFrame;
    r32 SecondsPerFrame;
    b32 TimelineShouldAdvance;
    
};

//////////////////////////
//
// Anim Layers Array

internal anim_layer_array
AnimLayerArray_Create(gs_memory_arena* Storage, u32 CountMax)
{
    anim_layer_array Result = {0};
    Result.CountMax = CountMax;
    Result.Values = PushArray(Storage, anim_layer, Result.CountMax);
    return Result;
}

internal u32
AnimLayerArray_Push(anim_layer_array* Array, anim_layer Value)
{
    Assert(Array->Count < Array->CountMax);
    u32 Index = Array->Count++;
    Array->Values[Index] = Value;
    return Index;
}

internal u32
AnimLayerArray_Remove(anim_layer_array* Array, u32 Index)
{
    Assert(Index < Array->Count);
    for (u32 i = Index; i < Array->Count - 1; i++)
    {
        Array->Values[i] = Array->Values[i + 1];
    }
}

//////////////////////////
//
// Animation Array

internal animation_array
AnimationArray_Create(gs_memory_arena* Storage, u32 CountMax)
{
    animation_array Result = {0};
    Result.CountMax = CountMax;
    Result.Values = PushArray(Storage, animation, Result.CountMax);
    return Result;
}

internal u32
AnimationArray_Push(animation_array* Array, animation Value)
{
    Assert(Array->Count < Array->CountMax);
    u32 Index = Array->Count++;
    Array->Values[Index] = Value;
    return Index;
}

//////////////////////////
//
// Animation

internal u32
Animation_AddLayer(animation* Animation, anim_layer Layer)
{
    return AnimLayerArray_Push(&Animation->Layers, Layer);
}

internal u32
Animation_AddLayer (animation* Animation, gs_string Name, blend_mode BlendMode, animation_system* System)
{
    anim_layer NewLayer = {0};
    NewLayer.Name = PushStringF(System->Storage, 256, "%S", Name);
    NewLayer.BlendMode = BlendMode;
    
    return Animation_AddLayer(Animation, NewLayer);
}

internal void
Animation_RemoveLayer (animation* Animation, u32 LayerIndex)
{
    AnimLayerArray_Remove(&Animation->Layers, LayerIndex);
    for (u32 i = Animation->Blocks.Used -= 1; i >= 0; i--)
    {
        gs_list_entry<animation_block>* Entry = Animation->Blocks.GetEntryAtIndex(i);
        if (EntryIsFree(Entry)) { continue; }
        
        animation_block* Block = &Entry->Value;
        if (Block->Layer > LayerIndex)
        {
            Block->Layer -= 1;
        }
        else if (Block->Layer == LayerIndex)
        {
            Animation->Blocks.FreeElementAtIndex(i);
        }
    }
}

//////////////////////////
//
//

internal u32
SecondsToFrames(r32 Seconds, animation_system System)
{
    u32 Result = Seconds * (1.0f / System.SecondsPerFrame);
    return Result;
}

inline b32
FrameIsInRange(frame_range Range, s32 Frame)
{
    b32 Result = (Frame >= Range.Min) && (Frame <= Range.Max);
    return Result;
}

internal u32
GetFrameCount(frame_range Range)
{
    u32 Result = (u32)Max(0, Range.Max - Range.Min);
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
Animation_AddBlock(animation* Animation, u32 StartFrame, s32 EndFrame, u32 AnimationProcHandle, u32 LayerIndex)
{
    Assert(LayerIndex < Animation->Layers.Count);
    
    animation_block NewBlock = {0};
    NewBlock.Range.Min = StartFrame;
    NewBlock.Range.Max = EndFrame;
    NewBlock.AnimationProcHandle = AnimationProcHandle;
    NewBlock.Layer = LayerIndex;
    
    gs_list_handle Result = Animation->Blocks.PushElementOnList(NewBlock);
    return Result;
}

internal void
Animation_RemoveBlock(animation* Animation, gs_list_handle AnimationBlockHandle)
{
    Assert(ListHandleIsValid(AnimationBlockHandle));
    Animation->Blocks.FreeElementWithHandle(AnimationBlockHandle);
}

// Layers

// System

internal animation*
AnimationSystem_GetActiveAnimation(animation_system* System)
{
    // TODO(pjs): need a way to specify the active animation
    return System->Animations.Values + 0;
}

internal animation_frame
AnimationSystem_CalculateAnimationFrame(animation_system* System, gs_memory_arena* Arena)
{
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
    
    animation_frame Result = {0};
    Result.BlocksCountMax = ActiveAnim->Layers.Count;
    Result.Blocks = PushArray(Arena, animation_block, Result.BlocksCountMax);
    Result.BlocksFilled = PushArray(Arena, b8, Result.BlocksCountMax);
    
    for (u32 i = 0; i < ActiveAnim->Blocks.Used; i++)
    {
        gs_list_entry<animation_block>* BlockEntry = ActiveAnim->Blocks.GetEntryAtIndex(i);
        if (EntryIsFree(BlockEntry)) { continue; }
        
        animation_block Block = BlockEntry->Value;
        
        if (FrameIsInRange(Block.Range, System->CurrentFrame)){ continue; }
        
        Result.BlocksFilled[Block.Layer] = true;
        Result.Blocks[Block.Layer] = Block;
        Result.BlocksCount++;
    }
    
    return Result;
}

#define FOLDHAUS_ANIMATION
#endif // FOLDHAUS_ANIMATION