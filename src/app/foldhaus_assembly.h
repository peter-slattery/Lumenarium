//
// File: foldhaus_assembly.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_H

struct led
{
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

// NOTE(Peter): This structure is so we can keep track of
// what LEDs output to which DMX universe. You don't need
// to use it anywhere else, as all the data for patterns,
// colors, and groups is/will be stored elsewhere.
struct leds_in_universe_range
{
    s32 RangeStart;
    s32 RangeOnePastLast;
    s32 Universe;
};

struct assembly_led_buffer
{
    u32 LEDCount;
    pixel* Colors;
    led* LEDs;
};

struct v2_tag
{
    u64 NameHash;
    u64 ValueHash;
};

struct v2_strip
{
    s32 ControlBoxID; // TODO(Peter): I don't think we need this anymore
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
    
    u32 StripCount;
    v2_strip* Strips;
    
    s32 LedCountTotal;
    assembly_led_buffer LEDBuffer;
    
    u32 LEDUniverseMapCount;
    leds_in_universe_range* LEDUniverseMap;
};


#define FOLDHAUS_ASSEMBLY_H
#endif // FOLDHAUS_ASSEMBLY_H