#include "../meta/gs_meta_lexer.h"

#include "gs_font.h"
#include "interface.h"

#include "foldhaus_network_ordering.h"
#include "foldhaus_sacn.h"

struct led
{
    s32 Index;
    v4 Position;
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
#include "foldhaus_node.h"
#include "assembly_parser.cpp"
#include "test_patterns.h"
#include "kraftwerks_patterns.h"
#include "foldhaus_interface.h"

typedef struct app_state app_state;

#include "foldhaus_command_dispatch.h"
#include "foldhaus_command_dispatch.cpp"
#include "foldhaus_operation_mode.h"

#include "foldhaus_text_entry.h"

#include "foldhaus_default_nodes.h"
#include "generated/foldhaus_nodes_generated.cpp"
#include "foldhaus_search_lister.h"

struct app_state
{
    memory_arena* Permanent; 
    memory_arena* Transient;
    memory_arena  SACNMemory;
    
    streaming_acn SACN;
    s32 TotalLEDsCount;
    led_buffer* LEDBufferList;
    
    camera Camera;
    r32 PixelsToWorldScale;
    
    operation_mode_system Modes;
    input_command_registry DefaultInputCommandRegistry;
    input_command_queue CommandQueue;
    text_entry ActiveTextEntry;
    
    // TODO(Peter): Make this dynamic. We want them contiguous in memory since we'll be accessing them
    // mostly by looping through them. On the other hand, I don't expect there to ever be more than 100 
    // of them at once. 
#define ASSEMBLY_LIST_LENGTH 32
    assembly AssemblyList[ASSEMBLY_LIST_LENGTH];
    s32 AssembliesUsed;
    
    node_list* NodeList;
    interface_node* OutputNode;
    
    node_render_settings NodeRenderSettings;
    bitmap_font* Font;
    interface_config Interface;
};

internal void OpenColorPicker(app_state* State, v4* Address);

#include "foldhaus_node.cpp"
#include "foldhaus_debug_visuals.h"
#include "foldhaus_sacn_view.cpp"
#include "foldhaus_text_entry.cpp"
#include "foldhaus_search_lister.cpp"

#include "foldhaus_interface.cpp"