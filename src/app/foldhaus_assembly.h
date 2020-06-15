//
// File: foldhaus_assembly.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_H

struct led
{
    // TODO(Peter): Pretty sure we don't need this. led and pixel are always parallel arrays
    s32 Index;
    v4 Position;
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
    led* Leds;
};

struct led_system
{
    platform_memory_handler PlatformMemory;
    
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

struct v2_strip
{
    s32 ControlBoxID; // TODO(Peter): I don't think we need this anymore
    // TODO(Peter): Add in info for Serial, ArtNet, etc.
    s32 StartUniverse;
    s32 StartChannel;
    
    // TODO(Peter): When we create more ways to calculate points, this needs to become
    // a type enum and a union
    v3 StartPosition;
    v3 EndPosition;
    
    u32 LedCount;
    u32* LedLUT;
    
    u32 TagsCount;
    v2_tag* Tags;
};

struct assembly
{
    memory_arena Arena;
    
    string Name;
    string FilePath;
    
    r32 Scale;
    s32 LedCountTotal;
    u32 LedBufferIndex;
    
    u32 StripCount;
    v2_strip* Strips;
};

struct assembly_array
{
    u32 CountMax;
    u32 Count;
    assembly* Values;
};

#define FOLDHAUS_ASSEMBLY_H
#endif // FOLDHAUS_ASSEMBLY_H