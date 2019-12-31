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
    
    s32 LEDCount;
    pixel* Colors;
    led* LEDs;
    
    s32 LEDUniverseMapCount;
    leds_in_universe_range* LEDUniverseMap;
};
