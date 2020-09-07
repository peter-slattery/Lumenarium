//
// File: foldhaus_assembly.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_H

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

struct led_strip_list
{
    u32 Count;
    u32 CountMax;
    u32* StripIndices;
};

struct assembly
{
    gs_memory_arena Arena;
    
    gs_string Name;
    gs_string FilePath;
    
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