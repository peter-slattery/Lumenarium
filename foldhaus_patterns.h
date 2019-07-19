typedef s32 pattern_id;

typedef struct pattern_led pattern_led;

#define PATTERN_INIT_PROC(name) void name(led_pattern* Pattern, memory_arena* Storage)
typedef PATTERN_INIT_PROC(pattern_init_proc);

#define PATTERN_PUSH_COLOR_PROC(name) void name(led* LED, sacn_pixel* Colors, u8 R, u8 G, u8 B)
typedef PATTERN_PUSH_COLOR_PROC(pattern_push_color_proc);

#define PATTERN_UPDATE_PROC(name) void name(led* LEDs, sacn_pixel* Colors, s32 LEDCount, void* Memory, r32 DeltaTime, pattern_push_color_proc PushColor)
typedef PATTERN_UPDATE_PROC(pattern_update_proc);

struct pattern_registry_entry
{
    char* Name;
    pattern_init_proc* Init;
    pattern_update_proc* Update;
};

struct pattern_led
{
    s32 LocationInSendBuffer;
    u32 Color;
    v3 Position;
};

enum pattern_selector_combine_operation
{
    PatternSelectorCombine_Invalid,
    
    PatternSelectorCombine_Override,
    PatternSelectorCombine_Add,
    PatternSelectorCombine_Multiply,
    
    PatternSelectorCombine_Count,
};

char* PatternSelectorOperationsText[] = {
    "Invalid",
    
    "Override",
    "Add",
    "Multiply",
    
    "Count",
};

struct pattern_index_id_key
{
    s32 Index;
    pattern_id ID;
};

struct led_pattern
{
    pattern_id ID;
    
    void* Memory;
    
    pattern_update_proc* UpdateProc;
    char* Name;
};

struct led_pattern_system
{
    // TODO(Peter): Need to think about how this grows
    led_pattern* Patterns;
    s32 PatternsUsed;
    s32 PatternsMax;
    
    // TODO(Peter): Need to think about how this can have a free list as well of some sort.
    // It might be inexpensive enough to just compress this memory whenever we remove a pattern
    memory_arena PatternWorkingMemoryStorage;
    
    
    pattern_id IDAccumulator;
};
