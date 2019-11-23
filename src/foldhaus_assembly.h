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

#define AssemblySize(leds, name_length) ((sizeof(led) + sizeof(pixel)) * (leds)) + name_length;
struct assembly
{
    memory_arena Memory;
    s32 MemorySize;
    u8* MemoryBase;
    
    string Name;
    string FilePath;
    
    s32 LEDCount;
    pixel* Colors;
    led* LEDs;
};
