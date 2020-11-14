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
AnimationSystem_RenderToLedBuffers(animation_system* System, assembly_array Assemblies,
                                   led_system* LedSystem,
                                   animation_pattern* Patterns,
                                   gs_memory_arena* Transient)
{
    s32 CurrentFrame = System->CurrentFrame;
    r32 FrameTime = CurrentFrame * System->SecondsPerFrame;
    
    animation* ActiveAnim = AnimationSystem_GetActiveAnimation(System);
    animation_frame CurrFrame = AnimationSystem_CalculateAnimationFrame(System, Transient);
    
    led_buffer* LayerLedBuffers = PushArray(Transient, led_buffer, CurrFrame.BlocksCountMax);
    
    for (u32 AssemblyIndex = 0; AssemblyIndex < Assemblies.Count; AssemblyIndex++)
    {
        assembly* Assembly = &Assemblies.Values[AssemblyIndex];
        led_buffer* AssemblyLedBuffer = LedSystemGetBuffer(LedSystem, Assembly->LedBufferIndex);
        
        // Create the LayerLEDBuffers
        for (u32 Layer = 0; Layer < CurrFrame.BlocksCountMax; Layer++)
        {
            led_buffer TempBuffer = {};
            TempBuffer.LedCount = AssemblyLedBuffer->LedCount;
            TempBuffer.Positions = AssemblyLedBuffer->Positions;
            TempBuffer.Colors = PushArray(Transient, pixel, TempBuffer.LedCount);
            LedBuffer_ClearToBlack(&TempBuffer);
            
            LayerLedBuffers[Layer] = TempBuffer;
        }
        
        // Render Each layer's block to the appropriate temp buffer
        for (u32 Layer = 0; Layer < CurrFrame.BlocksCountMax; Layer++)
        {
            led_buffer TempBuffer = LayerLedBuffers[Layer];
            
            if (!CurrFrame.BlocksFilled[Layer]) { continue; }
            animation_block Block = CurrFrame.Blocks[Layer];
            
            u32 FramesIntoBlock = CurrentFrame - Block.Range.Min;
            r32 SecondsIntoBlock = FramesIntoBlock * System->SecondsPerFrame;
            
            // :AnimProcHandle
            u32 AnimationProcIndex = Block.AnimationProcHandle - 1;
            animation_proc* AnimationProc = Patterns[AnimationProcIndex].Proc;
            AnimationProc(&TempBuffer, *Assembly, SecondsIntoBlock, Transient);
        }
        
        // Consolidate Temp Buffers
        // We do this in reverse order so that they go from top to bottom
        for (u32 Layer = 0; Layer < CurrFrame.BlocksCountMax; Layer++)
        {
            if (!CurrFrame.BlocksFilled[Layer]) { continue; }
            
            led_blend_proc* Blend = LedBlend_GetProc(ActiveAnim->Layers.Values[Layer].BlendMode);
            Assert(Blend != 0);
            for (u32 LED = 0; LED < AssemblyLedBuffer->LedCount; LED++)
            {
                pixel A = AssemblyLedBuffer->Colors[LED];
                pixel B = LayerLedBuffers[Layer].Colors[LED];
                AssemblyLedBuffer->Colors[LED] = Blend(A, B);
            }
        }
    }
}

#define FOLDHAUS_ANIMATION_RENDERER_CPP
#endif // FOLDHAUS_ANIMATION_RENDERER_CPP