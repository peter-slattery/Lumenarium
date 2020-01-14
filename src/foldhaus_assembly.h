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

struct leds_in_universe_range
{
    s32 RangeStart;
    s32 RangeOnePastLast;
    s32 Universe;
};

struct assembly
{
    memory_arena Arena;
    
    string Name;
    string FilePath;
    
    u32 LEDCount;
    pixel* Colors;
    led* LEDs;
    
    u32 LEDUniverseMapCount;
    leds_in_universe_range* LEDUniverseMap;
};


#define FOLDHAUS_ASSEMBLY_H
#endif // FOLDHAUS_ASSEMBLY_H