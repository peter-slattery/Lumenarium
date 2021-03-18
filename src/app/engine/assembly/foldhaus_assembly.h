//
// File: foldhaus_assembly.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_H

enum network_protocol
{
    NetworkProtocol_SACN,
    NetworkProtocol_ArtNet,
    NetworkProtocol_UART,
    
    NetworkProtocol_Count,
};

union pixel
{
    struct
    {
        u8 R;
        u8 G;
        u8 B;
    };
    u8 Channels[3];
};

struct led_buffer
{
    u32 LedCount;
    pixel* Colors;
    v4* Positions;
};

struct led_system
{
    gs_allocator PlatformMemory;
    
    u32 BuffersCountMax;
    u32 BuffersCount;
    led_buffer* Buffers;
    
    u32 LedsCountTotal;
};

struct v2_tag
{
    u64 NameHash;
    u64 ValueHash;
};

struct strip_sacn_addr
{
    s32 StartUniverse;
    s32 StartChannel;
};

struct strip_uart_addr
{
    u8 Channel;
    
    gs_string ComPort;
    // This may not be used based on the value of the parent
    // assembly's NetworkPortMode field
};

enum strip_gen_method
{
    StripGeneration_InterpolatePoints,
    StripGeneration_Sequence,
    
    StripGeneration_Count,
};

typedef struct strip_gen_data strip_gen_data;

struct strip_gen_interpolate_points
{
    v3 StartPosition;
    v3 EndPosition;
    u32 LedCount;
};

struct strip_gen_sequence
{
    strip_gen_data* Elements;
    u32 ElementsCount;
};

struct strip_gen_data
{
    strip_gen_method Method;
    
    strip_gen_interpolate_points InterpolatePoints;
    strip_gen_sequence Sequence;
};

struct v2_strip
{
    s32 ControlBoxID; // TODO(Peter): I don't think we need this anymore
    
    strip_sacn_addr SACNAddr;
    strip_uart_addr UARTAddr;
    
    strip_gen_data GenerationData;
    
    u32 LedCount;
    u32* LedLUT;
    
    u32 TagsCount;
    v2_tag* Tags;
};

struct led_strip_list
{
    u32 Count;
    u32 CountMax;
    u32* StripIndices;
};

enum network_port_mode
{
    // This enum defines the scope which contains what network
    // port each address should be sent over.
    
    NetworkPortMode_GlobalPort,
    // GlobalPort means that the port is defined in the assembly structure
    
    NetworkPortMode_PortPerStrip,
    // PortPerStrip means that the address stored in the strip structure
    // should be used, and each strip might have a different port
    
    NetworkPortMode_Count,
};

struct assembly
{
    gs_memory_arena Arena;
    
    gs_string Name;
    gs_string FilePath;
    
    r32 Scale;
    v3 Center;
    s32 LedCountTotal;
    u32 LedBufferIndex;
    
    u32 StripCount;
    v2_strip* Strips;
    
    network_protocol OutputMode;
    network_port_mode NetPortMode;
    gs_string UARTComPort;
};

struct assembly_array
{
    u32 CountMax;
    u32 Count;
    assembly* Values;
};

typedef pixel led_blend_proc(pixel A, pixel B, u8* UserData);

internal led_buffer*
LedSystemGetBuffer(led_system* System, u32 Index)
{
    led_buffer* Result = &System->Buffers[Index];
    return Result;
}

internal void
LedBuffer_ClearToBlack(led_buffer* Buffer)
{
    for (u32 i = 0; i < Buffer->LedCount; i++)
    {
        Buffer->Colors[i].R = 0;
        Buffer->Colors[i].G = 0;
        Buffer->Colors[i].B = 0;
    }
}

internal void
LedBuffer_Copy(led_buffer From, led_buffer* To)
{
    Assert(From.LedCount == To->LedCount);
    u32 LedCount = To->LedCount;
    for (u32 i = 0; i < LedCount; i++)
    {
        To->Colors[i] = From.Colors[i];
    }
}

internal void
LedBuffer_Blend(led_buffer A, led_buffer B, led_buffer* Dest, led_blend_proc* BlendProc, u8* UserData)
{
    Assert(A.LedCount == B.LedCount);
    Assert(Dest->LedCount == A.LedCount);
    Assert(BlendProc);
    
    u32 LedCount = Dest->LedCount;
    for (u32 i = 0; i < LedCount; i++)
    {
        pixel PA = A.Colors[i];
        pixel PB = B.Colors[i];
        Dest->Colors[i] = BlendProc(PA, PB, UserData);
    }
}

internal led_buffer
LedBuffer_CreateCopyCleared (led_buffer Buffer, gs_memory_arena* Arena)
{
    led_buffer Result = {};
    Result.LedCount = Buffer.LedCount;
    Result.Positions = Buffer.Positions;
    Result.Colors = PushArray(Arena, pixel, Buffer.LedCount);
    LedBuffer_ClearToBlack(&Result);
    return Result;
}

internal u32
StripGenData_CountLeds(strip_gen_data Data)
{
    u32 Result = 0;
    
    switch (Data.Method)
    {
        case StripGeneration_InterpolatePoints:
        {
            Result += Data.InterpolatePoints.LedCount;
        }break;
        
        case StripGeneration_Sequence:
        {
            for (u32 i = 0; i < Data.Sequence.ElementsCount; i++)
            {
                Result += StripGenData_CountLeds(Data.Sequence.Elements[i]);
            }
        }break;
        
        InvalidDefaultCase;
    }
    
    return Result;
}

internal bool
AssemblyStrip_HasTagValue(v2_strip Strip, u64 NameHash, u64 ValueHash)
{
    bool Result = false;
    for (u32 i = 0; i < Strip.TagsCount; i++)
    {
        v2_tag TagAt = Strip.Tags[i];
        if (TagAt.NameHash == NameHash)
        {
            // NOTE(pjs): We can pass an empty string to the Value parameter,
            // and it will match all values of Tag
            if (ValueHash == 0 || ValueHash == TagAt.ValueHash)
            {
                Result = true;
                break;
            }
        }
    }
    return Result;
}

internal bool
AssemblyStrip_HasTagValueSLOW(v2_strip Strip, char* Name, char* Value)
{
    u64 NameHash = HashDJB2ToU32(Name);
    u64 ValueHash = HashDJB2ToU32(Value);
    return AssemblyStrip_HasTagValue(Strip, NameHash, ValueHash);
}

#define FOLDHAUS_ASSEMBLY_H
#endif // FOLDHAUS_ASSEMBLY_H