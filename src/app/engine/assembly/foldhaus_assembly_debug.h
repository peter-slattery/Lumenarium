//
// File: foldhaus_assembly_debug.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef FOLDHAUS_ASSEMBLY_DEBUG_H

enum override_type
{
    ADS_Override_None,
    
    ADS_Override_Strip,
    ADS_Override_SoloStrip,
    ADS_Override_AllRed,
    ADS_Override_AllGreen,
    ADS_Override_AllBlue,
    
    ADS_Override_Count,
};

global gs_const_string OverrideTypeStrings[] = {
    LitString("Override_None"),
    LitString("Override_Strip"),
    LitString("Override_SoloStrip" ),
    LitString("Override_AllRed" ),
    LitString("Override_AllGreen" ),
    LitString("Override_AllBlue" ),
    LitString("Override_Count"),
};

struct assembly_debug_state
{
    override_type Override;
    u32 TargetAssembly;
    u32 TargetStrip;
    pixel TargetColor;
};

internal void
AssemblyDebug_OverrideOutput(assembly_debug_state State, assembly_array Assemblies, led_system LedSystem)
{
    if (State.Override == ADS_Override_None) return;
    State.TargetColor = pixel{255,255,255};
    
    assembly Assembly = Assemblies.Values[State.TargetAssembly];
    led_buffer LedBuffer = LedSystem.Buffers[Assembly.LedBufferIndex];
    
    switch (State.Override)
    {
        case ADS_Override_Strip:
        {
            v2_strip Strip = Assembly.Strips[State.TargetStrip];
            for (u32 i = 0; i < Strip.LedCount; i++)
            {
                u32 LedIdx = Strip.LedLUT[i];
                LedBuffer.Colors[LedIdx] = State.TargetColor;
            }
        }break;
        
        case ADS_Override_SoloStrip:
        {
            for (u32 s = 0; s < Assembly.StripCount; s++)
            {
                v2_strip Strip = Assembly.Strips[s];
                
                pixel Color = pixel{0,0,0};
                if (s == State.TargetStrip)
                {
                    Color = State.TargetColor;
                }
                
                for (u32 i = 0; i < Strip.LedCount; i++)
                {
                    u32 LedIdx = Strip.LedLUT[i];
                    LedBuffer.Colors[LedIdx] = Color;
                }
            }
        }break;
        
        case ADS_Override_AllRed:
        {
            for (u32 s = 0; s < Assembly.StripCount; s++)
            {
                v2_strip Strip = Assembly.Strips[s];
                for (u32 i = 0; i < Strip.LedCount; i++)
                {
                    u32 LedIdx = Strip.LedLUT[i];
                    LedBuffer.Colors[LedIdx] = pixel{255, 0, 0};
                }
            }
        }break;
        
        case ADS_Override_AllGreen:
        {
            for (u32 s = 0; s < Assembly.StripCount; s++)
            {
                v2_strip Strip = Assembly.Strips[s];
                for (u32 i = 0; i < Strip.LedCount; i++)
                {
                    u32 LedIdx = Strip.LedLUT[i];
                    LedBuffer.Colors[LedIdx] = pixel{0, 255, 0};
                }
            }
        }break;
        
        case ADS_Override_AllBlue:
        {
            for (u32 s = 0; s < Assembly.StripCount; s++)
            {
                v2_strip Strip = Assembly.Strips[s];
                for (u32 i = 0; i < Strip.LedCount; i++)
                {
                    u32 LedIdx = Strip.LedLUT[i];
                    LedBuffer.Colors[LedIdx] = pixel{0, 0, 255};
                }
            }
        }break;
        
        case ADS_Override_None:
        {
        }break;
        
        InvalidDefaultCase;
    }
    
    if (State.Override )
    {
        
    }
}


#define FOLDHAUS_ASSEMBLY_DEBUG_H
#endif // FOLDHAUS_ASSEMBLY_DEBUG_H