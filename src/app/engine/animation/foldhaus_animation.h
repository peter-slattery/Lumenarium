//
// File: foldhaus_animation.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ANIMATION

#define ANIMATION_PROC(name) void name(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
typedef ANIMATION_PROC(animation_proc);

struct frame_range
{
  s32 Min;
  s32 Max;
};

struct animation_pattern_handle
{
  s32 IndexPlusOne;
};

// NOTE(pjs): An animation block is a time range paired with an
// animation_pattern (see below). While a timeline's current time
// is within the range of a block, that particular block's animation
// will run
struct animation_block
{
  frame_range Range;
  animation_pattern_handle AnimationProcHandle;
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

// TODO(pjs): This really doesn't belong here
global gs_string BlendModeStrings[] = {
  MakeString("Overwrite"),
  MakeString("Add"),
  MakeString("Multiply"),
  MakeString("Count"),
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

// NOTE(pjs): An animation is a stack of layers, each of which
// is a timeline of animation blocks.
struct animation
{
  gs_string Name;
  
  anim_layer_array Layers;
  animation_block_array Blocks_;
  
  frame_range PlayableRange;
  
  // The information / path to the file where this animation is to be saved / where it is loaded from
  gs_file_info FileInfo;
};

struct animation_handle
{
  s32 Index;
};

struct animation_handle_array
{
  u32 Count;
  animation_handle* Handles;
};

internal animation_handle InvalidAnimHandle () { return { -1 }; }
internal bool IsValid (animation_handle H) { return H.Index >= 0; }
internal void Clear (animation_handle* H) { H->Index = -1; }
internal bool AnimHandlesAreEqual (animation_handle A, animation_handle B)
{
  return A.Index == B.Index;
}

struct animation_array
{
  animation* Values;
  u32 Count;
  u32 CountMax;
};

struct animation_layer_frame
{
  animation_block Hot;
  bool HasHot;
  
  animation_block NextHot;
  bool HasNextHot;
  
  r32 NextHotOpacity;
  
  blend_mode BlendMode;
};

// NOTE(pjs): This is an evaluated frame - across all layers in an
// animation, these are the blocks that need to be run
struct animation_frame
{
  animation_layer_frame* Layers;
  u32 LayersCount;
};

enum animation_repeat_mode
{
  AnimationRepeat_Single,
  AnimationRepeat_Loop,
  AnimationRepeat_Invalid,
};

global gs_const_string AnimationRepeatModeStrings[] = {
  ConstString("Repeat Single"),
  ConstString("Loop"),
  ConstString("Invalid"),
};

struct animation_fade_group
{
  animation_handle From;
  animation_handle To;
  r32 FadeElapsed;
  r32 FadeDuration;
};

#define ANIMATION_SYSTEM_LAYERS_MAX 128
#define ANIMATION_SYSTEM_BLOCKS_MAX 128
struct animation_system
{
  gs_memory_arena* Storage;
  animation_array Animations;
  
  animation_repeat_mode RepeatMode;
  animation_handle_array Playlist;
  u32 PlaylistAt;
  r32 PlaylistFadeTime;
  
  // NOTE(Peter): The frame currently being displayed/processed. you
  // can see which frame you're on by looking at the time slider on the timeline
  // panel
  animation_fade_group ActiveFadeGroup;
  
  r32 SecondsOnCurrentFrame;
  s32 CurrentFrame;
  s32 LastUpdatedFrame;
  r32 SecondsPerFrame;
  b32 TimelineShouldAdvance;
  u32 UpdatesThisFrame;
  
  // Settings
  bool Multithreaded;
};

// NOTE(pjs): A Pattern is a named procedure which can be used as
// an element of an animation. Patterns are sequenced on a timeline
// and blended via layers to create an animation
struct animation_pattern
{
  char* Name;
  s32 NameLength;
  animation_proc* Proc;
  bool Multithreaded;
};

struct animation_pattern_array
{
  animation_pattern* Values;
  u32 Count;
  u32 CountMax;
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
// Patterns List

internal animation_pattern_array
Patterns_Create(gs_memory_arena* Arena, s32 CountMax)
{
  animation_pattern_array Result = {0};
  Result.CountMax = CountMax;
  Result.Values = PushArray(Arena, animation_pattern, Result.CountMax);
  return Result;
}

#define PATTERN_MULTITHREADED true
#define PATTERN_SINGLETHREADED false

#define Patterns_PushPattern(array, proc, multithread) \
Patterns_PushPattern_((array), (proc), Stringify(proc), sizeof(Stringify(proc)) - 1, (multithread))

internal void
Patterns_PushPattern_(animation_pattern_array* Array, animation_proc* Proc, char* Name, u32 NameLength, bool Multithreaded)
{
  Assert(Array->Count < Array->CountMax);
  
  animation_pattern Pattern = {0};
  Pattern.Name = Name;
  Pattern.NameLength = NameLength;
  Pattern.Proc = Proc;
  Pattern.Multithreaded = Multithreaded;
  
  Array->Values[Array->Count++] = Pattern;
}

internal animation_pattern_handle
Patterns_IndexToHandle(s32 Index)
{
  animation_pattern_handle Result = {};
  Result.IndexPlusOne = Index + 1;
  return Result;
}

internal bool
IsValid(animation_pattern_handle Handle)
{
  bool Result = Handle.IndexPlusOne > 0;
  return Result;
}

internal animation_pattern
Patterns_GetPattern(animation_pattern_array Patterns, animation_pattern_handle Handle)
{
  animation_pattern Result = {0};
  if (Handle.IndexPlusOne > 0)
  {
    u32 Index = Handle.IndexPlusOne - 1;
    Assert(Index < Patterns.Count);
    Result = Patterns.Values[Index];
  }
  return Result;
}

//////////////////////////
//
// Anim Block Array

internal animation_block_array
AnimBlockArray_Create(gs_memory_arena* Storage, u32 CountMax)
{
  animation_block_array Result = {0};
  Result.CountMax = Max(CountMax, 32);
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
  Result.CountMax = Max(CountMax, 32);
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

internal animation_handle
AnimationArray_Push(animation_array* Array, animation Value)
{
  Assert(Array->Count < Array->CountMax);
  animation_handle Result = {0};
  Result.Index = Array->Count++;
  Array->Values[Result.Index] = Value;
  return Result;
}

internal animation*
AnimationArray_Get(animation_array Array, animation_handle Handle)
{
  DEBUG_TRACK_FUNCTION;
  
  animation* Result = 0;
  if (IsValid(Handle) && Handle.Index < (s32)Array.Count)
  {
    Result = Array.Values + Handle.Index;
  }
  return Result;
}

internal animation*
AnimationArray_GetSafe(animation_array Array, animation_handle Handle)
{
  Assert(IsValid(Handle));
  Assert(Handle.Index < (s32)Array.Count);
  return AnimationArray_Get(Array, Handle);
}

//////////////////////////
//
// Animation

typedef struct animation_desc
{
  u32 NameSize;
  char* Name;
  
  u32 LayersCount;
  u32 BlocksCount;
  
  u32 MinFrames;
  u32 MaxFrames;
} animation_desc;

internal animation
Animation_Create(animation_desc Desc, animation_system* System)
{
  animation Result = {};
  u32 NameLen = Desc.NameSize;
  if (Desc.Name)
  {
    NameLen = Max(CStringLength(Desc.Name), NameLen);
    Result.Name = PushStringF(System->Storage, NameLen, "%s", Desc.Name);
  } else {
    Result.Name = PushStringF(System->Storage, NameLen, "[New Animation]");
  }
  
  Result.Layers = AnimLayerArray_Create(System->Storage, Desc.LayersCount);
  Result.Blocks_ = AnimBlockArray_Create(System->Storage, Desc.BlocksCount);
  Result.PlayableRange.Min = Desc.MinFrames;
  Result.PlayableRange.Max = Desc.MaxFrames;
  return Result;
}

internal handle
Animation_AddBlock(animation* Animation, u32 StartFrame, s32 EndFrame, animation_pattern_handle AnimationProcHandle, u32 LayerIndex)
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

inline frame_range
FrameRange_Overlap(frame_range A, frame_range B)
{
  frame_range Result = {};
  
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

// Fade Group

internal bool
AnimationFadeGroup_ShouldRender (animation_fade_group FadeGroup)
{
  return IsValid(FadeGroup.From);
}

internal void
AnimationFadeGroup_Advance(animation_fade_group* Group)
{
  Group->From = Group->To;
  Clear(&Group->To);
  Group->FadeElapsed = 0;
  Group->FadeDuration = 0;
}

internal void
AnimationFadeGroup_Update(animation_fade_group* Group, r32 DeltaTime)
{
  if (IsValid(Group->To))
  {
    r32 FadeBefore = Group->FadeElapsed;
    Group->FadeElapsed += DeltaTime;
    
    if (Group->FadeElapsed >= Group->FadeDuration)
    {
      AnimationFadeGroup_Advance(Group);
    }
  }
}

internal void
AnimationFadeGroup_FadeTo(animation_fade_group* Group, animation_handle To, r32 Duration)
{
  if (IsValid(Group->From))
  {
    // complete current fade if there is one in progress
    if (IsValid(Group->To))
    {
      AnimationFadeGroup_Advance(Group);
    }
    
    Group->To = To;
    Group->FadeDuration = Duration;
  }
  else
  {
    Group->From = To;
  }
}

// System

struct animation_system_desc
{
  gs_memory_arena* Storage;
  u32 AnimArrayCount;
  r32 SecondsPerFrame;
};

internal animation_system
AnimationSystem_Init(animation_system_desc Desc)
{
  animation_system Result = {};
  Result.Storage = Desc.Storage;
  Result.Animations = AnimationArray_Create(Result.Storage, Desc.AnimArrayCount);
  Result.SecondsPerFrame = Desc.SecondsPerFrame;
  
  Clear(&Result.ActiveFadeGroup.From);
  Clear(&Result.ActiveFadeGroup.To);
  Result.ActiveFadeGroup.FadeElapsed = 0;
  
  // Settings
  Result.Multithreaded = false;
  
  return Result;
}

internal animation*
AnimationSystem_GetActiveAnimation(animation_system* System)
{
  return AnimationArray_Get(System->Animations, System->ActiveFadeGroup.From);
}

internal animation_frame
AnimationSystem_CalculateAnimationFrame(animation_system* System,
                                        animation* Animation,
                                        gs_memory_arena* Arena)
{
  DEBUG_TRACK_FUNCTION;
  
  animation_frame Result = {0};
  Result.LayersCount = Animation->Layers.Count;
  Result.Layers = PushArray(Arena, animation_layer_frame, Result.LayersCount);
  ZeroArray(Result.Layers, animation_layer_frame, Result.LayersCount);
  
  for (u32 l = 0; l < Animation->Layers.Count; l++)
  {
    animation_layer_frame* Layer = Result.Layers + l;
    Layer->BlendMode = Animation->Layers.Values[l].BlendMode;
  }
  
  for (u32 i = 0; i < Animation->Blocks_.Count; i++)
  {
    animation_block Block = Animation->Blocks_.Values[i];
    
    if (FrameIsInRange(Block.Range, System->CurrentFrame))
    {
      animation_layer_frame* Layer = Result.Layers + Block.Layer;
      if (Layer->HasHot)
      {
        // NOTE(pjs): With current implementation, we don't allow
        // animations to hvae more than 2 concurrent blocks in the
        // timeline
        Assert(!Layer->HasNextHot);
        
        // NOTE(pjs): Make sure that Hot comes before NextHot
        if (Layer->Hot.Range.Min < Block.Range.Min)
        {
          Layer->NextHot = Block;
        }
        else
        {
          Layer->NextHot = Layer->Hot;
          Layer->Hot = Block;
        }
        Layer->HasNextHot = true;
        
        frame_range BlendRange = {};
        BlendRange.Min = Layer->NextHot.Range.Min;
        BlendRange.Max = Layer->Hot.Range.Max;
        Layer->NextHotOpacity = FrameToPercentRange(System->CurrentFrame, BlendRange);
      }
      else
      {
        Layer->Hot = Block;
        Layer->NextHotOpacity = 0.0f;
        Layer->HasHot = true;
      }
    }
  }
  
  return Result;
}

internal void
AnimationSystem_Update(animation_system* System, r32 DeltaTime)
{
  if (!System->TimelineShouldAdvance) { return; }
  if (!AnimationFadeGroup_ShouldRender(System->ActiveFadeGroup)) { return; }
  
  System->UpdatesThisFrame = 0;
  
  AnimationFadeGroup_Update(&System->ActiveFadeGroup, DeltaTime);
  
  animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
  if (ActiveAnim)
  {
    System->SecondsOnCurrentFrame += DeltaTime;
    while (System->SecondsOnCurrentFrame > System->SecondsPerFrame)
    {
      System->CurrentFrame += 1;
      System->SecondsOnCurrentFrame -= System->SecondsPerFrame;
      System->UpdatesThisFrame += 1;
    }
    
    // Loop back to the beginning
    if (System->CurrentFrame > ActiveAnim->PlayableRange.Max)
    {
      // NOTE(PS): There's no long term reason why this needs to be true
      // but I don't want to implement dealing with PlayableRanges that
      // don't start at zero right now becuse there's literally no reason
      // I can think of where that's useful.
      Assert(ActiveAnim->PlayableRange.Min == 0);
      
      s32 FramesPastEnd = System->CurrentFrame;
      while (FramesPastEnd > ActiveAnim->PlayableRange.Max)
      {
        FramesPastEnd -= ActiveAnim->PlayableRange.Max;
      }
      
      switch (System->RepeatMode)
      {
        case AnimationRepeat_Single:
        {
          System->CurrentFrame = 0;
        }break;
        
        case AnimationRepeat_Loop:
        {
          Assert(System->Playlist.Count > 0);
          u32 NextIndex = System->PlaylistAt;
          System->PlaylistAt = (System->PlaylistAt + 1) % System->Playlist.Count;
          animation_handle Next = System->Playlist.Handles[NextIndex];
          
          AnimationFadeGroup_FadeTo(&System->ActiveFadeGroup,
                                    Next,
                                    System->PlaylistFadeTime);
          System->CurrentFrame = 0;
        }break;
        
        InvalidDefaultCase;
      }
    }
  }
}

internal void
AnimationSystem_FadeToPlaylist(animation_system* System, animation_handle_array Playlist)
{
  System->Playlist = Playlist;
  System->PlaylistAt = 0;
  
  if (System->Playlist.Count > 0)
  {
    AnimationFadeGroup_FadeTo(&System->ActiveFadeGroup, Playlist.Handles[0], System->PlaylistFadeTime);
  }
}

inline bool
AnimationSystem_NeedsRender(animation_system System)
{
  bool Result = (System.CurrentFrame != System.LastUpdatedFrame);
  return Result;
}

inline r32
AnimationSystem_GetCurrentTime(animation_system System)
{
  r32 Result = System.CurrentFrame * System.SecondsPerFrame;
  return Result;
}

#define FOLDHAUS_ANIMATION
#endif // FOLDHAUS_ANIMATION