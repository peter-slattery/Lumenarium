#include "../meta/gs_meta_lexer.h"

#include "gs_font.h"
#include "interface.h"

#include "foldhaus_network_ordering.h"
#include "dmx/dmx.h"
#include "sacn/sacn.h"

#include "foldhaus_assembly.h"

#include "assembly_parser.h"
#include "foldhaus_node.h"
#include "assembly_parser.cpp"
#include "test_patterns.h"
#include "foldhaus_interface.h"

typedef struct app_state app_state;

#include "foldhaus_command_dispatch.h"
#include "foldhaus_command_dispatch.cpp"
#include "foldhaus_operation_mode.h"

#include "foldhaus_text_entry.h"

#include "foldhaus_search_lister.h"

enum network_protocol
{
    NetworkProtocol_SACN,
    NetworkProtocol_ArtNet,
    
    NetworkProtocol_Count,
};

struct app_state
{
    memory_arena Permanent; 
    memory_arena Transient;
    
    s32 NetworkProtocolHeaderSize;
    network_protocol NetworkProtocol;
    
    streaming_acn SACN;
    
    s32 TotalLEDsCount;
    
    assembly_array AssemblyList;
    array_entry_handle_contiguous_array ActiveAssemblyIndecies;
    
    camera Camera;
    r32 PixelsToWorldScale;
    
    operation_mode_system Modes;
    input_command_registry DefaultInputCommandRegistry;
    input_command_queue CommandQueue;
    text_entry ActiveTextEntry;
    
    bitmap_font* Font;
    interface_config Interface;
    
    r32 GreenIter;
    r32 BlueIter;
    r32 RedIter;
};

internal void OpenColorPicker(app_state* State, v4* Address);

#include "foldhaus_assembly.cpp"

#include "foldhaus_debug_visuals.h"
//#include "foldhaus_sacn_view.cpp"
#include "foldhaus_text_entry.cpp"
#include "foldhaus_search_lister.cpp"

#include "foldhaus_interface.cpp"