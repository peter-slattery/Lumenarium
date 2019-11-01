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
#include "foldhaus_node.h"
#include "assembly_parser.cpp"
#include "test_patterns.h"
#include "kraftwerks_patterns.h"
#include "foldhaus_interface.h"

typedef struct app_state app_state;

#include "foldhaus_command_dispatch.h"
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
    
    camera Camera;
    
    operation_mode_system Modes;
    
    input_command_registry InputCommandRegistry;
    // TODO(Peter): At the moment this is only still here because text input into nodes utilizes it. 
    // Get rid of this once Modes are working and you can switch all text input over to various modes
    input_command_registry NodeListerCommandRegistry;
    // NOTE(Peter): stores the address of the command registry to be activated next frame.
    // was having a problem where switching command registry's in the middle of the loop trying to 
    // execute commands was causing problems. 
    input_command_registry* NextCommandRegistry;
    input_command_registry* ActiveCommands;
    
    input_command_queue CommandQueue;
    text_entry ActiveTextEntry;
    
    streaming_acn SACN;
    s32 TotalLEDsCount;
    led_buffer* LEDBufferList;
    
    // TODO(Peter): Make this dynamic. We want them contiguous in memory since we'll be accessing them
    // mostly by looping through them. On the other hand, I don't expect there to ever be more than 100 
    // of them at once. 
#define ASSEMBLY_LIST_LENGTH 32
    assembly AssemblyList[ASSEMBLY_LIST_LENGTH];
    s32 AssembliesUsed;
    
    bitmap_font* Font;
    interface_config Interface;
    
    r32 PixelsToWorldScale;
    v4 Camera_StartDragPos;
    
    node_list* NodeList;
    interface_node* OutputNode;
    
    node_render_settings NodeRenderSettings;
    
    
    
    string GeneralPurposeSearchString;
};

// TODO(Peter): Once rendering nodes becomes an operation_mode you can get rid of this pre-declaration
internal void OpenColorPicker(app_state* State, v4* Address);

#include "foldhaus_debug_visuals.h"
#include "foldhaus_sacn_view.cpp"
#include "foldhaus_command_dispatch.cpp"
#include "foldhaus_node.cpp"
#include "foldhaus_text_entry.cpp"
#include "foldhaus_search_lister.cpp"

#include "foldhaus_interface.cpp"