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

struct animation_block_array
{
    u32* Generations;
    animation_block* Values;
    u32 Count;
    u32 CountMax;
};

enum blend_mode
{
    BlendMode_Overwrite,
    BlendMode_Add,
    BlendMode_Multiply,
    BlendMode_Count,
};

global gs_const_string BlendModeStrings[] = {
    ConstString("Overwrite"),
    ConstString("Add"),
    ConstString("Multiply"),
    ConstString("Count"),
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
    gs_string Name;
    
    anim_layer_array Layers;
    animation_block_array Blocks_;
    
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
    
    // NOTE(Peter): The frame currently being displayed/processed. you
    // can see which frame you're on by looking at the time slider on the timeline
    // panel
    u32 ActiveAnimationIndex;
    s32 CurrentFrame;
    s32 LastUpdatedFrame;
    r32 SecondsPerFrame;
    b32 TimelineShouldAdvance;
    
};

// TODO(pjs): Better name - something like animation_prototype
struct animation_pattern
{
    char* Name;
    s32 NameLength;
    animation_proc* Proc;
};

// Serialization

enum animation_field
{
    AnimField_FileIdent,
    AnimField_AnimName,
    AnimField_LayersCount,
    AnimField_BlocksCount,
    
    AnimField_PlayableRange,
    AnimField_PlayableRangeMin,
    AnimField_PlayableRangeMax,
    
    AnimField_LayersArray,
    AnimField_Layer,
    AnimField_LayerName,
    AnimField_LayerBlendMode,
    
    AnimField_BlocksArray,
    AnimField_Block,
    AnimField_BlockFrameRange,
    AnimField_BlockFrameRangeMin,
    AnimField_BlockFrameRangeMax,
    AnimField_BlockLayerIndex,
    AnimField_BlockAnimName,
    
    AnimField_Count,
};

global gs_const_string AnimationFieldStrings[] = {
    ConstString("lumenarium_animation_file"), // AnimField_FileIdent
    ConstString("animation_name"),// AnimField_AnimName
    ConstString("layers_count"),// AnimField_LayersCount
    ConstString("blocks_count"),// AnimField_BlocksCount
    
    ConstString("playable_range"),// AnimField_PlayableRange
    ConstString("min"),// AnimField_PlayableRangeMin
    ConstString("max"),// AnimField_PlayableRangeMax
    
    ConstString("layers"),// AnimField_LayersArray
    ConstString("layer"),// AnimField_Layer
    ConstString("name"),// AnimField_LayerName
    ConstString("blend"),// AnimField_LayerBlendMode
    
    ConstString("blocks"),// AnimField_BlocksArray
    ConstString("block"),// AnimField_Block
    ConstString("frame_range"),// AnimField_BlockFrameRange
    ConstString("min"),// AnimField_BlockFrameRangeMin
    ConstString("max"),// AnimField_BlockFrameRangeMax
    ConstString("layer_index"),// AnimField_BlockLayerIndex
    ConstString("animation_name"),// AnimField_BlockAnimName
};

//////////////////////////
//
// Anim Block Array

internal animation_block_array
AnimBlockArray_Create(gs_memory_arena* Storage, u32 CountMax)
{
    animation_block_array Result = {0};
    Result.CountMax = CountMax;
    Result.Values = PushArray(Storage, animation_block, Result.CountMax);
    Result.Generations = PushArray(Storage, u32, Result.CountMax);
    return Result;
}

internal handle
AnimBlockArray_Push(animation_block_array* Array, animation_block Value)
{
    Assert(Array->Count < Array->CountMax);
    handle Result = {0};
    Result.Index = Array->Count++;
    // NOTE(pjs): pre-increment so that generation 0 is always invalid
    Result.Generation = ++Array->Generations[Result.Index];
    
    Array->Values[Result.Index] = Value;
    
    return Result;
}

internal void
AnimBlockArray_Remove(animation_block_array* Array, handle Handle)
{
    Assert(Handle.Index < Array->Count);
    Assert(Handle_IsValid(Handle));
    Array->Generations[Handle.Index]++;
}

internal void
AnimBlockArray_RemoveAt(animation_block_array* Array, u32 Index)
{
    Assert(Index < Array->Count);
    
    handle Handle = {};
    Handle.Index = Index;
    Handle.Generation = Array->Generations[Index];
    AnimBlockArray_Remove(Array, Handle);
}

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

internal void
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

internal handle
Animation_AddBlock(animation* Animation, u32 StartFrame, s32 EndFrame, u32 AnimationProcHandle, u32 LayerIndex)
{
    Assert(LayerIndex < Animation->Layers.Count);
    
    animation_block NewBlock = {0};
    NewBlock.Range.Min = StartFrame;
    NewBlock.Range.Max = EndFrame;
    NewBlock.AnimationProcHandle = AnimationProcHandle;
    NewBlock.Layer = LayerIndex;
    
    handle Handle = AnimBlockArray_Push(&Animation->Blocks_, NewBlock);
    return Handle;
}

internal void
Animation_RemoveBlock(animation* Animation, handle AnimHandle)
{
    AnimBlockArray_Remove(&Animation->Blocks_, AnimHandle);
}

internal animation_block*
Animation_GetBlockFromHandle(animation* Animation, handle AnimHandle)
{
    animation_block* Result = 0;
    
    if (AnimHandle.Generation != 0 &&
        Animation->Blocks_.Generations[AnimHandle.Index] == AnimHandle.Generation)
    {
        Result = Animation->Blocks_.Values + AnimHandle.Index;
    }
    
    return Result;
}

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
    for (u32 i = Animation->Blocks_.Count - 1; i >= 0; i--)
    {
        animation_block* Block = Animation->Blocks_.Values + i;
        if (Block->Layer > LayerIndex)
        {
            Block->Layer -= 1;
        }
        else if (Block->Layer == LayerIndex)
        {
            AnimBlockArray_RemoveAt(&Animation->Blocks_, i);
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

inline bool
FrameIsInRange(frame_range Range, s32 Frame)
{
    bool Result = (Frame >= Range.Min) && (Frame <= Range.Max);
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

// Layers

// System

internal animation*
AnimationSystem_GetActiveAnimation(animation_system* System)
{
    // TODO(pjs): need a way to specify the active animation
    return System->Animations.Values + System->ActiveAnimationIndex;
}

internal animation_frame
AnimationSystem_CalculateAnimationFrame(animation_system* System, gs_memory_arena* Arena)
{
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
    
    animation_frame Result = {0};
    Result.BlocksCountMax = ActiveAnim->Layers.Count;
    Result.Blocks = PushArray(Arena, animation_block, Result.BlocksCountMax);
    Result.BlocksFilled = PushArray(Arena, b8, Result.BlocksCountMax);
    ZeroArray(Result.BlocksFilled, b8, Result.BlocksCountMax);
    
    for (u32 i = 0; i < ActiveAnim->Blocks_.Count; i++)
    {
        animation_block Block = ActiveAnim->Blocks_.Values[i];
        
        if (FrameIsInRange(Block.Range, System->CurrentFrame))
        {
            Result.BlocksFilled[Block.Layer] = true;
            Result.Blocks[Block.Layer] = Block;
            Result.BlocksCount++;
        }
    }
    
    return Result;
}

#define FOLDHAUS_ANIMATION
#endif // FOLDHAUS_ANIMATION