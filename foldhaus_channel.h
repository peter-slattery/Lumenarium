#if 0 // USAGE CODE

{
    channel* FirstChannel = AddChannel(State, State->Channels);
    
    channel* ActiveChannel = GetChannelAtIndex(State->Channels, ActiveIndex);
    AddPattern(State, ActiveChannel, PatternSpec);
    
    patterns_need_update UpdatePatternsList = UdpateAllChannels(ChannelSystem, State->Transient);
    
    for (s32 i = 0; i < UpdatePatternsList.Count; i++)
    {
        UpdateActivePatterns(State, Patterns, UpdatePatternsList.PatternIDs[i]);
    }
    
    pattern* Pattern = ...;
    DeletePattern(State, Pattern);
    
    RemoveChannel(FirstChannel, State->Channels);
    RemoveChannel(2, State->Channels);
}
#endif

struct pattern_transition
{
    r32 Duration;
    r32 TimeElapsed;
};

enum channel_blend_mode
{
    ChannelBlend_Invalid,
    
    ChannelBlend_Override,
    ChannelBlend_Add,
    ChannelBlend_Multiply,
    
    ChannelBlend_Count,
};

char* METAChannelBlendModeNames[] = {
    "Invalid", //ChannelBlend_Invalid
    "Override", //ChannelBlend_Override
    "Add", // ChannelBlend_Add
    "Multiply", //ChannelBlend_Multiply
    "Count", //ChannelBlend_Count
};

global_variable s32 ChannelIDAccumulator;

// TODO(Peter): This number is gonna have to get bigger
#define CHANNEL_MAX_PATTERNS 8
struct led_channel
{
    s32 ChannelID;
    
    pattern_index_id_key* Patterns;
    // TODO(Peter): Rename this once we get patterns in their own system. All patterns in this
    // list are active. ATM this is just here for legacy reasons.
    s32 ActivePatterns;
    // TODO(Peter): and this should probably be CurrentPatternIndex, or HotPatternIndex
    s32 ActivePatternIndex;
    
    // TODO(Peter): extend this to be able to have different kinds of transitions
    // including, most importantly, being able to fade between patterns
    pattern_transition Transition;
    channel_blend_mode BlendMode;
    
    led_channel* Next;
};

struct led_channel_system
{
    led_channel* Channels;
    s32 ChannelCount;
    
    // TODO(Peter): think about ways this can give back to the main pool
    // Like, if we just stay within initial storage, no problem. But if we grow really big,
    // then shrink really small, we should probably give some of it back to the main app.
    // maybe by keeping track of FreeList size...
    // TODO(Peter): Also need to think about how memory_arenas get bigger...
    memory_arena Storage;
    led_channel* FreeList;
};

struct pattern_update_list_entry
{
    pattern_index_id_key Key;
    pattern_push_color_proc* PushColorProc;
};

struct patterns_update_list
{
    pattern_update_list_entry* Patterns;
    s32 Size;
    s32 Used;
    patterns_update_list* Next;
};
