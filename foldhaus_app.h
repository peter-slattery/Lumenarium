#include "../meta/gs_meta_lexer.h"

#include "gs_font.h"
#include "interface.h"

#include "foldhaus_network_ordering.h"
#include "foldhaus_sacn.h"

#define LED_BUFFER_SIZE 256
struct led
{
    s32 Index;
    v3 Position;
    m44 PositionMatrix;
};

struct led_buffer
{
    sacn_pixel* Colors;
    led* LEDs;
    s32 Count;
    s32 Max;
    
    led_buffer* Next;
};

#define CalculateMemorySizeForAssembly(leds, name_length) ((sizeof(led) + sizeof(sacn_pixel)) * (leds)) + sizeof(led_buffer) + name_length;
struct assembly
{
    s32 MemorySize;
    u8* MemoryBase;
    
    string Name;
    string FilePath;
    led_buffer* LEDBuffer;
    
    // Memory managed by the SACN system
    sacn_universe_buffer* Universes;
    sacn_send_buffer* SendBuffer;
};

#include "assembly_parser.h"

// TODO(Peter): remove this, and get pattern_system.h outta here!!
typedef struct led_pattern led_pattern;

#include "foldhaus_node.h"

#include "foldhaus_patterns.h"
#include "foldhaus_channel.h"
#include "foldhaus_patterns.cpp"
#include "foldhaus_channel.cpp"

#include "assembly_parser.cpp"

#include "test_patterns.h"
#include "patterns_registry.h"
#include "foldhaus_interface.h"

typedef struct app_state app_state;

#include "foldhaus_debug_visuals.h"
#include "foldhaus_command_dispatch.h"

#include "generated/foldhaus_nodes_generated.cpp"

struct app_state
{
    memory_arena* Permanent; 
    memory_arena* Transient;
    memory_arena  SACNMemory;
    
    render_texture* LoadedTextures;
    s32 LoadedTexturesSize;
    s32 LoadedTexturesUsed;
    
    camera Camera;
    
    input_command_registry InputCommandRegistry;
    
    streaming_acn SACN;
    s32 TotalLEDsCount;
    led_buffer* LEDBufferList;
    
    // TODO(Peter): Make this dynamic. WE want them contiguous in memory since we'll be accessing them
    // mostly by looping through them. On the other hand, I don't expect there to ever be more than 100 
    // of them at once. 
#define ASSEMBLY_LIST_LENGTH 32
    assembly AssemblyList[ASSEMBLY_LIST_LENGTH];
    s32 AssembliesUsed;
    
    //environment Environment;
    led_channel_system ChannelSystem;
    led_pattern_system PatternSystem;
    
    bitmap_font* Font;
    interface_state InterfaceState;
    interface_config Interface;
    
    r32 InterfaceYMax;
    r32 PixelsToWorldScale;
    v4 Camera_StartDragPos;
    
    b32 DrawUniverseOutputDisplay;
    v2 UniverseOutputDisplayOffset;
    r32 UniverseOutputDisplayZoom;
    
    b32 InterfaceShowNodeList;
    v2 NodeListMenuPosition;
    
    node_list* NodeList;
    node_interaction NodeInteraction;
    node_render_settings NodeRenderSettings;
    interface_node* OutputNode;
    
    v4* ColorPickerEditValue;
};

#include "foldhaus_sacn_view.cpp"
#include "foldhaus_command_dispatch.cpp"
#include "foldhaus_node.cpp"
#include "foldhaus_interface.cpp"
