//
// File: foldhaus_animation_renderer.cpp
// Author: Peter Slattery
// Creation Date: 2020-11-14
//
#ifndef FOLDHAUS_ANIMATION_RENDERER_CPP

internal pixel
LedBlend_Overwrite(pixel PixelA, pixel PixelB, u8* UserData)
{
    return PixelB;
}

internal pixel
LedBlend_Lerp(pixel PixelA, pixel PixelB, u8* UserData)
{
    r32 BOpacity = *(r32*)UserData;
    
    pixel Result = {};
    
    r32 AOpacity = 1.0f - BOpacity;
    Result.R = (u8)((PixelA.R * AOpacity) + (PixelB.R * BOpacity));
    Result.G = (u8)((PixelA.G * AOpacity) + (PixelB.G * BOpacity));
    Result.B = (u8)((PixelA.B * AOpacity) + (PixelB.B * BOpacity));
    return Result;
}

internal pixel
LedBlend_Add(pixel PixelA, pixel PixelB, u8* UserData)
{
    pixel Result = {};
    
    u32 R = (u32)PixelA.R + (u32)PixelB.R;
    u32 G = (u32)PixelA.G + (u32)PixelB.G;
    u32 B = (u32)PixelA.B + (u32)PixelB.B;
    
    Result.R = (u8)Min(R, (u32)255);
    Result.G = (u8)Min(G, (u32)255);
    Result.B = (u8)Min(B, (u32)255);
    
    return Result;
}

internal pixel
LedBlend_Multiply(pixel PixelA, pixel PixelB, u8* UserData)
{
    pixel Result = {};
    
    r32 DR = (r32)PixelA.R / 255.f;
    r32 DG = (r32)PixelA.G / 255.f;
    r32 DB = (r32)PixelA.B / 255.f;
    
    r32 SR = (r32)PixelB.R / 255.f;
    r32 SG = (r32)PixelB.G / 255.f;
    r32 SB = (r32)PixelB.B / 255.f;
    
    Result.R = (u8)((DR * SR) * 255.f);
    Result.G = (u8)((DG * SG) * 255.f);
    Result.B = (u8)((DB * SB) * 255.f);
    
    return Result;
}

internal pixel
LedBlend_Overlay(pixel PixelA, pixel PixelB, u8* UserData)
{
    pixel Result = {};
    return Result;
}

internal led_blend_proc*
LedBlend_GetProc(blend_mode BlendMode)
{
    led_blend_proc* Result = 0;
    switch (BlendMode)
    {
        case BlendMode_Overwrite: { Result = LedBlend_Overwrite; }break;
        case BlendMode_Add:       { Result = LedBlend_Add;       }break;
        case BlendMode_Multiply:  { Result = LedBlend_Multiply;  }break;
        InvalidDefaultCase;
    }
    return Result;
}

struct pattern_args
{
    assembly Assembly;
    gs_memory_arena* Transient;
    u8* UserData;
};

struct render_anim_to_led_buffer_job_data
{
    animation_pattern Pattern;
    led_buffer Buffer;
    led_buffer_range BufferRange;
    pattern_args PatternArgs;
    r32 SecondsIntoBlock;
};

internal void
AnimationSystem_RenderAnimationToLedBufferJob(gs_thread_context Context, gs_data Data)
{
    render_anim_to_led_buffer_job_data JobData = *(render_anim_to_led_buffer_job_data*)Data.Memory;
    JobData.Pattern.Proc(&JobData.Buffer, 
                         JobData.BufferRange,
                         JobData.PatternArgs.Assembly, 
                         JobData.SecondsIntoBlock, 
                         JobData.PatternArgs.Transient, 
                         JobData.PatternArgs.UserData);
}

#define MULTITHREAD_PATTERN_RENDERING 1

internal void
AnimationSystem_BeginRenderBlockToLedBuffer(animation_system* System, animation_block Block, led_buffer* Buffer, animation_pattern_array Patterns, pattern_args PatternArgs,
                                            context Context)
{
    DEBUG_TRACK_FUNCTION;
    
    u32 FramesIntoBlock = System->CurrentFrame - Block.Range.Min;
    r32 SecondsIntoBlock = FramesIntoBlock * System->SecondsPerFrame;
    
    animation_pattern Pattern = Patterns_GetPattern(Patterns, Block.AnimationProcHandle);
    
#if MULTITHREAD_PATTERN_RENDERING
    u32 JobsCount = 4;
    u32 LedsPerJob = Buffer->LedCount / JobsCount;
    
    for (u32 i = 0; i < JobsCount; i++)
    {
        gs_data Data = PushSizeToData(Context.ThreadContext.Transient, sizeof(render_anim_to_led_buffer_job_data));
        render_anim_to_led_buffer_job_data* JobData = (render_anim_to_led_buffer_job_data*)Data.Memory;
        JobData->Pattern = Pattern;
        JobData->Buffer = *Buffer;
        JobData->BufferRange.First = LedsPerJob * i;
        JobData->BufferRange.OnePastLast = LedsPerJob * (i + 1);
        JobData->PatternArgs = PatternArgs;
        JobData->SecondsIntoBlock = SecondsIntoBlock;
        
        Context.GeneralWorkQueue->PushWorkOnQueue(Context.GeneralWorkQueue,
                                                  (thread_proc*)AnimationSystem_RenderAnimationToLedBufferJob,
                                                  Data,
                                                  ConstString("Render Pattern To Buffer"));
    }
#else
    Pattern.Proc(Buffer, PatternArgs.Assembly, SecondsIntoBlock, PatternArgs.Transient, PatternArgs.UserData);
#endif
}

internal void
AnimationSystem_EndRenderBlockToLedBuffer (context Context)
{
#if MULTITHREAD_PATTERN_RENDERING
    Context.GeneralWorkQueue->CompleteQueueWork(Context.GeneralWorkQueue, Context.ThreadContext);
#endif
}

// NOTE(pjs): This mirrors animation_layer_frame to account
// for overlapping
struct layer_led_buffer
{
    led_buffer HotBuffer;
    led_buffer NextHotBuffer;
};

internal led_buffer
RenderAnimationToLedBuffer (animation_system* System,
                            pattern_args PatternArgs,
                            animation_frame CurrFrame,
                            layer_led_buffer* LayerBuffers,
                            led_buffer* AssemblyLedBuffer,
                            animation_pattern_array Patterns,
                            gs_memory_arena* Transient,
                            context Context)
{
    led_buffer AccBuffer = LedBuffer_CreateCopyCleared(*AssemblyLedBuffer, Transient);
    
    // Create the LayerLEDBuffers
    for (u32 Layer = 0; Layer < CurrFrame.LayersCount; Layer++)
    {
        layer_led_buffer TempBuffer = {};
        
        if (CurrFrame.Layers[Layer].HasHot)
        {
            TempBuffer.HotBuffer = LedBuffer_CreateCopyCleared(*AssemblyLedBuffer, Transient);
            
            if (CurrFrame.Layers[Layer].HasNextHot)
            {
                TempBuffer.NextHotBuffer = LedBuffer_CreateCopyCleared(*AssemblyLedBuffer, Transient);
            }
        }
        
        LayerBuffers[Layer] = TempBuffer;
    }
    
    // Render Each layer's block to the appropriate temp buffer
    for (u32 Layer = 0; Layer < CurrFrame.LayersCount; Layer++)
    {
        animation_layer_frame LayerFrame = CurrFrame.Layers[Layer];
        if (LayerFrame.HasHot)
        {
            led_buffer TempBuffer = LayerBuffers[Layer].HotBuffer;
            animation_block Block = LayerFrame.Hot;
            AnimationSystem_BeginRenderBlockToLedBuffer(System, Block, &TempBuffer,  Patterns, PatternArgs, Context);
        }
        
        if (LayerFrame.HasNextHot)
        {
            led_buffer TempBuffer = LayerBuffers[Layer].NextHotBuffer;
            animation_block Block = LayerFrame.NextHot;
            AnimationSystem_BeginRenderBlockToLedBuffer(System, Block, &TempBuffer,  Patterns, PatternArgs, Context);
        }
        
        AnimationSystem_EndRenderBlockToLedBuffer(Context);
    }
    
    // Blend together any layers that have a hot and next hot buffer
    for (u32 Layer = 0; Layer < CurrFrame.LayersCount; Layer++)
    {
        animation_layer_frame LayerFrame = CurrFrame.Layers[Layer];
        layer_led_buffer LayerBuffer = LayerBuffers[Layer];
        if (LayerFrame.HasNextHot)
        {
            LedBuffer_Blend(LayerBuffer.HotBuffer,
                            LayerBuffer.NextHotBuffer,
                            &LayerBuffer.HotBuffer,
                            LedBlend_Lerp,
                            (u8*)&LayerFrame.NextHotOpacity);
        }
    }
    
    // Consolidate Temp Buffers back into AssemblyLedBuffer
    // We do this in reverse order so that they go from top to bottom
    for (u32 Layer = 0; Layer < CurrFrame.LayersCount; Layer++)
    {
        if (CurrFrame.Layers[Layer].HasHot)
        {
            led_blend_proc* Blend = LedBlend_GetProc(CurrFrame.Layers[Layer].BlendMode);
            LedBuffer_Blend(AccBuffer,
                            LayerBuffers[Layer].HotBuffer,
                            &AccBuffer,
                            Blend,
                            0);
        }
    }
    
    return AccBuffer;
}

internal void
AnimationSystem_RenderToLedBuffers(animation_system* System, assembly_array Assemblies,
                                   led_system* LedSystem,
                                   animation_pattern_array Patterns,
                                   gs_memory_arena* Transient,
                                   context Context,
                                   u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    
    s32 CurrentFrame = System->CurrentFrame;
    r32 FrameTime = CurrentFrame * System->SecondsPerFrame;
    
#if 1
    animation_array Animations = System->Animations;
    animation_fade_group FadeGroup = System->ActiveFadeGroup;
    
    animation* FromAnim = AnimationArray_Get(Animations, FadeGroup.From);
    animation_frame FromFrame = AnimationSystem_CalculateAnimationFrame(System, FromAnim, Transient);
    layer_led_buffer* FromLayerBuffers = PushArray(Transient, layer_led_buffer, FromFrame.LayersCount);
    
    animation* ToAnim = AnimationArray_Get(Animations, FadeGroup.To);
    animation_frame ToFrame = {0};
    layer_led_buffer* ToLayerBuffers = 0;
    if (ToAnim)
    {
        ToFrame = AnimationSystem_CalculateAnimationFrame(System, ToAnim, Transient);
        ToLayerBuffers = PushArray(Transient, layer_led_buffer, ToFrame.LayersCount);
    }
    
    for (u32 AssemblyIndex = 0; AssemblyIndex < Assemblies.Count; AssemblyIndex++)
    {
        assembly Assembly = Assemblies.Values[AssemblyIndex];
        led_buffer* AssemblyLedBuffer = LedSystemGetBuffer(LedSystem, Assembly.LedBufferIndex);
        
        pattern_args PatternArgs = {};
        PatternArgs.Assembly = Assembly;
        PatternArgs.Transient = Transient;
        PatternArgs.UserData = UserData;
        
        led_buffer FromBuffer = RenderAnimationToLedBuffer(System,
                                                           PatternArgs,
                                                           FromFrame,
                                                           FromLayerBuffers,
                                                           AssemblyLedBuffer,
                                                           Patterns,
                                                           Transient,
                                                           Context);
        led_buffer ConsolidatedBuffer = FromBuffer;
        
        if (ToAnim) {
            led_buffer ToBuffer = RenderAnimationToLedBuffer(System,
                                                             PatternArgs,
                                                             ToFrame,
                                                             ToLayerBuffers,
                                                             AssemblyLedBuffer,
                                                             Patterns,
                                                             Transient,
                                                             Context);
            
            r32 BlendPercent = FadeGroup.FadeElapsed / FadeGroup.FadeDuration;
            LedBuffer_Blend(FromBuffer, ToBuffer, &ConsolidatedBuffer, LedBlend_Lerp, (u8*)&BlendPercent);
        }
        
        LedBuffer_Copy(ConsolidatedBuffer, AssemblyLedBuffer);
    }
    
#else
    
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
    animation_frame CurrFrame = AnimationSystem_CalculateAnimationFrame(System, ActiveAnim, Transient);
    
    for (u32 AssemblyIndex = 0; AssemblyIndex < Assemblies.Count; AssemblyIndex++)
    {
        assembly Assembly = Assemblies.Values[AssemblyIndex];
        led_buffer* AssemblyLedBuffer = LedSystemGetBuffer(LedSystem, Assembly.LedBufferIndex);
        
        pattern_args PatternArgs = {};
        PatternArgs.Assembly = Assembly;
        PatternArgs.Transient = Transient;
        PatternArgs.UserData = UserData;
        
        led_buffer AccBuffer = RenderAnimationToLedBuffer(System,
                                                          PatternArgs,
                                                          CurrFrame,
                                                          LayerBuffers,
                                                          AssemblyLedBuffer,
                                                          Patterns,
                                                          Transient);
        LedBuffer_Copy(AccBuffer, AssemblyLedBuffer);
    }
    
#endif
    
    System->LastUpdatedFrame = System->CurrentFrame;
}

#define FOLDHAUS_ANIMATION_RENDERER_CPP
#endif // FOLDHAUS_ANIMATION_RENDERER_CPP