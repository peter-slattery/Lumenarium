//
// File: foldhaus_animation_renderer.cpp
// Author: Peter Slattery
// Creation Date: 2020-11-14
//
#ifndef FOLDHAUS_ANIMATION_RENDERER_CPP

internal pixel
LedBlend_Overwrite(pixel PixelA, pixel PixelB)
{
    return PixelB;
}

internal pixel
LedBlend_Overwrite(pixel PixelA, pixel PixelB, r32 Opacity)
{
    pixel Result = {};
    
    r32 BOpacity = 1.0f - Opacity;
    Result.R = (u8)((PixelA.R * Opacity) + (PixelB.R * BOpacity));
    Result.G = (u8)((PixelA.G * Opacity) + (PixelB.G * BOpacity));
    Result.B = (u8)((PixelA.B * Opacity) + (PixelB.B * BOpacity));
    return Result;
}

internal pixel
LedBlend_Add(pixel PixelA, pixel PixelB)
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
LedBlend_Multiply(pixel PixelA, pixel PixelB)
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
LedBlend_Overlay(pixel PixelA, pixel PixelB)
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

internal void
AnimationSystem_RenderBlockToLedBuffer(animation_system* System, animation_block Block, led_buffer* Buffer,  assembly Assembly, animation_pattern_array Patterns,  gs_memory_arena* Transient,
                                       u8* UserData)
{
    u32 FramesIntoBlock = System->CurrentFrame - Block.Range.Min;
    r32 SecondsIntoBlock = FramesIntoBlock * System->SecondsPerFrame;
    
    animation_pattern Pattern = Patterns_GetPattern(Patterns, Block.AnimationProcHandle);
    Pattern.Proc(Buffer, Assembly, SecondsIntoBlock, Transient, UserData);
}

internal void
AnimationSystem_RenderToLedBuffers(animation_system* System, assembly_array Assemblies,
                                   led_system* LedSystem,
                                   animation_pattern_array Patterns,
                                   gs_memory_arena* Transient,
                                   u8* UserData)
{
    s32 CurrentFrame = System->CurrentFrame;
    r32 FrameTime = CurrentFrame * System->SecondsPerFrame;
    
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
    animation_frame CurrFrame = AnimationSystem_CalculateAnimationFrame(System, Transient);
    
    // NOTE(pjs): This mirrors animation_layer_frame to account
    // for overlapping
    struct layer_led_buffer
    {
        led_buffer HotBuffer;
        led_buffer NextHotBuffer;
    };
    
    layer_led_buffer* LayerBuffers = PushArray(Transient, layer_led_buffer, CurrFrame.LayersCount);
    
    for (u32 AssemblyIndex = 0; AssemblyIndex < Assemblies.Count; AssemblyIndex++)
    {
        assembly* Assembly = &Assemblies.Values[AssemblyIndex];
        led_buffer* AssemblyLedBuffer = LedSystemGetBuffer(LedSystem, Assembly->LedBufferIndex);
        
        // Create the LayerLEDBuffers
        for (u32 Layer = 0; Layer < CurrFrame.LayersCount; Layer++)
        {
            layer_led_buffer TempBuffer = {};
            if (CurrFrame.Layers[Layer].HasHot)
            {
                TempBuffer.HotBuffer.LedCount = AssemblyLedBuffer->LedCount;
                TempBuffer.HotBuffer.Positions = AssemblyLedBuffer->Positions;
                TempBuffer.HotBuffer.Colors = PushArray(Transient, pixel, TempBuffer.HotBuffer.LedCount);
                LedBuffer_ClearToBlack(&TempBuffer.HotBuffer);
            }
            
            if (CurrFrame.Layers[Layer].HasNextHot)
            {
                TempBuffer.NextHotBuffer.LedCount = AssemblyLedBuffer->LedCount;
                TempBuffer.NextHotBuffer.Positions = AssemblyLedBuffer->Positions;
                TempBuffer.NextHotBuffer.Colors = PushArray(Transient, pixel, TempBuffer.HotBuffer.LedCount);
                LedBuffer_ClearToBlack(&TempBuffer.NextHotBuffer);
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
                AnimationSystem_RenderBlockToLedBuffer(System, Block, &TempBuffer,  *Assembly, Patterns, Transient, UserData);
            }
            
            if (LayerFrame.HasNextHot)
            {
                led_buffer TempBuffer = LayerBuffers[Layer].NextHotBuffer;
                animation_block Block = LayerFrame.NextHot;
                AnimationSystem_RenderBlockToLedBuffer(System, Block, &TempBuffer,  *Assembly, Patterns, Transient, UserData);
            }
        }
        
        // Blend together any layers that have a hot and next hot buffer
        for (u32 Layer = 0; Layer < CurrFrame.LayersCount; Layer++)
        {
            animation_layer_frame LayerFrame = CurrFrame.Layers[Layer];
            layer_led_buffer LayerBuffer = LayerBuffers[Layer];
            if (LayerFrame.HasNextHot)
            {
                for (u32 LED = 0; LED < AssemblyLedBuffer->LedCount; LED++)
                {
                    pixel A = LayerBuffer.HotBuffer.Colors[LED];
                    pixel B = LayerBuffer.NextHotBuffer.Colors[LED];
                    LayerBuffer.HotBuffer.Colors[LED] = LedBlend_Overwrite(A, B, LayerFrame.HotOpacity);
                }
            }
        }
        
        // Consolidate Temp Buffers
        // We do this in reverse order so that they go from top to bottom
        for (u32 Layer = 0; Layer < CurrFrame.LayersCount; Layer++)
        {
            if (CurrFrame.Layers[Layer].HasHot)
            {
                led_blend_proc* Blend = LedBlend_GetProc(ActiveAnim->Layers.Values[Layer].BlendMode);
                Assert(Blend != 0);
                for (u32 LED = 0; LED < AssemblyLedBuffer->LedCount; LED++)
                {
                    pixel A = AssemblyLedBuffer->Colors[LED];
                    pixel B = LayerBuffers[Layer].HotBuffer.Colors[LED];
                    AssemblyLedBuffer->Colors[LED] = Blend(A, B);
                }
            }
        }
    }
    
    System->LastUpdatedFrame = System->CurrentFrame;
}

#define FOLDHAUS_ANIMATION_RENDERER_CPP
#endif // FOLDHAUS_ANIMATION_RENDERER_CPP