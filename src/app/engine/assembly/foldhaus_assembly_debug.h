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
    ADS_Override_AllOff,
    ADS_Override_AllWhite,
    ADS_Override_TagWhite,
    ADS_Override_TagStripWhite,
    ADS_Override_ChannelWhite,
    
    ADS_Override_Count,
};

global gs_const_string OverrideTypeStrings[] = {
    LitString("Override_None"),
    LitString("Override_Strip"),
    LitString("Override_SoloStrip" ),
    LitString("Override_AllRed" ),
    LitString("Override_AllGreen" ),
    LitString("Override_AllBlue" ),
    LitString("ADS_Override_AllOff" ),
    LitString("ADS_Override_AllWhite" ),
    LitString("ADS_Override_TagWhite" ),
    LitString("ADS_Override_TagStripWhite" ),
    LitString("ADS_Override_ChannelWhite," ),
    LitString("Override_Count"),
};

struct assembly_debug_state
{
    override_type Override;
    
    u32 TargetAssembly;
    u32 TargetStrip;
    
    gs_string TagName;
    gs_string TagValue;
    
    pixel TargetColor;
    
    u32 TargetChannel;
    
    u8 Brightness;
};

internal assembly_debug_state
AssemblyDebug_Create(gs_memory_arena* Storage)
{
    assembly_debug_state Result = {};
    Result.TagName = PushString(Storage, 256);
    Result.TagValue = PushString(Storage, 256);
    return Result;
}

internal void
AssemblyDebug_OverrideStripWithColor(v2_strip Strip, led_buffer LedBuffer, pixel Color)
{
    for (u32 i = 0; i < Strip.LedCount; i++)
    {
        u32 LedIdx = Strip.LedLUT[i];
        LedBuffer.Colors[LedIdx] = Color;
    }
}

internal void
AssemblyDebug_OverrideWithColor(assembly Assembly, led_buffer LedBuffer, pixel Color)
{
    for (u32 s = 0; s < Assembly.StripCount; s++)
    {
        v2_strip Strip = Assembly.Strips[s];
        AssemblyDebug_OverrideStripWithColor(Strip, LedBuffer, Color);
    }
}

internal void
AssemblyDebug_OverrideTagValueWithColor(assembly Assembly, led_buffer LedBuffer, pixel Color, gs_string TagName, gs_string TagValue)
{
    u64 NameHash = HashDJB2ToU32(StringExpand(TagName));
    u64 ValueHash = HashDJB2ToU32(StringExpand(TagValue));
    
    for (u32 s = 0; s < Assembly.StripCount; s++)
    {
        v2_strip Strip = Assembly.Strips[s];
        if (AssemblyStrip_HasTagValue(Strip, NameHash, ValueHash))
        {
            AssemblyDebug_OverrideStripWithColor(Strip, LedBuffer, Color);
        }
    }
}

internal void
AssemblyDebug_OverrideOutput(assembly_debug_state State, assembly_array Assemblies, led_system LedSystem)
{
    if (State.Override == ADS_Override_None) return;
    State.TargetColor = pixel{255,255,255};
    
    assembly Assembly = Assemblies.Values[State.TargetAssembly];
    led_buffer LedBuffer = LedSystem.Buffers[Assembly.LedBufferIndex];
    
    u8 V = State.Brightness;
    
    switch (State.Override)
    {
        case ADS_Override_Strip:
        {
            v2_strip Strip = Assembly.Strips[State.TargetStrip];
            AssemblyDebug_OverrideStripWithColor(Strip, LedBuffer, State.TargetColor);
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
                
                AssemblyDebug_OverrideStripWithColor(Strip, LedBuffer, Color);
            }
        }break;
        
        case ADS_Override_AllRed:
        {
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{V, 0, 0});
        }break;
        
        case ADS_Override_AllGreen:
        {
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{0, V, 0});
        }break;
        
        case ADS_Override_AllBlue:
        {
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{0, 0, V});
        }break;
        
        case ADS_Override_AllOff:
        {
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{0, 0, 0});
        }break;
        
        case ADS_Override_AllWhite:
        {
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{V, V, V});
        }break;
        
        case ADS_Override_TagWhite:
        {
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{0, 0, 0});
            AssemblyDebug_OverrideTagValueWithColor(Assembly, LedBuffer, pixel{255, 255, 255}, State.TagName, State.TagValue);
            
        }break;
        
        case ADS_Override_TagStripWhite:
        {
            u64 NameHash = HashDJB2ToU32(StringExpand(State.TagName));
            u64 ValueHash = HashDJB2ToU32(StringExpand(State.TagValue));
            
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{0, 0, 0});
            
            v2_strip Strip = Assembly.Strips[State.TargetStrip];
            if (AssemblyStrip_HasTagValue(Strip, NameHash, ValueHash))
            {
                AssemblyDebug_OverrideStripWithColor(Strip, LedBuffer,
                                                     pixel{255, 255, 255});
            }
        }break;
        
        case ADS_Override_ChannelWhite:
        {
            AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{0, 0, 0});
            for (u32 s = 0; s < Assembly.StripCount; s++)
            {
                v2_strip Strip = Assembly.Strips[s];
                if (Strip.UARTAddr.Channel == State.TargetChannel)
                {
                    AssemblyDebug_OverrideStripWithColor(Strip, LedBuffer, pixel{255, 255, 255});
                }
            }
        }break;
        
        case ADS_Override_None:
        {
        }break;
        
        InvalidDefaultCase;
    }
}


#define FOLDHAUS_ASSEMBLY_DEBUG_H
#endif // FOLDHAUS_ASSEMBLY_DEBUG_H