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


#include "foldhaus_panel.h"

#include "foldhaus_command_dispatch.h"
#include "foldhaus_command_dispatch.cpp"
#include "foldhaus_operation_mode.h"

#include "animation/foldhaus_animation.h"

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
    
    animation_system AnimationSystem;
    animation_block_handle SelectedAnimationBlockHandle;
    
    panel_layout PanelLayout;
};

internal void OpenColorPicker(app_state* State, v4* Address);

// BEGIN TEMPORARY PATTERNS
internal void
TestPatternOne(assembly* Assembly, r32 Time)
{
    for (s32 Range = 0; Range < Assembly->LEDUniverseMapCount; Range++)
    {
        leds_in_universe_range LEDUniverseRange = Assembly->LEDUniverseMap[Range];
        for (s32 LEDIdx = LEDUniverseRange.RangeStart;
             LEDIdx < LEDUniverseRange.RangeOnePastLast;
             LEDIdx++)
        {
            led LED = Assembly->LEDs[LEDIdx];
            Assembly->Colors[LED.Index].R = 255;
            Assembly->Colors[LED.Index].B = 255;
            Assembly->Colors[LED.Index].G = 255;
        }
        }
}

internal void
TestPatternTwo(assembly* Assembly, r32 Time)
{
    r32 PeriodicTime = (Time / PI) * 2;

    r32 ZeroOneSin = (GSSin(PeriodicTime) * .5f) + .5f;
r32 ZeroOneCos = (GSCos(PeriodicTime) * .5f) + .5f;
    pixel Color = { (u8)(ZeroOneSin * 255), 0, (u8)(ZeroOneCos * 255) };
    
    v4 Center = v4{0, 0, 0, 1};
    r32 ThetaZ = Time / 2;
    v4 Normal = v4{GSCos(ThetaZ), 0, GSSin(ThetaZ), 0}; // NOTE(Peter): dont' need to normalize. Should always be 1
    v4 Right = Cross(Normal, v4{0, 1, 0, 0});
    
    v4 FrontCenter = Center + (Normal * 25);
    v4 BackCenter = Center - (Normal * 25);
    
    r32 OuterRadiusSquared = 1000000;
    r32 InnerRadiusSquared = 0;
    
for (s32 Range = 0; Range < Assembly->LEDUniverseMapCount; Range++)
    {
    leds_in_universe_range LEDUniverseRange = Assembly->LEDUniverseMap[Range];
for (s32 LEDIdx = LEDUniverseRange.RangeStart;
             LEDIdx < LEDUniverseRange.RangeOnePastLast;
             LEDIdx++)
        {
            led LED = Assembly->LEDs[LEDIdx];

        v4 Position = LED.Position;
        
        v4 ToFront = Position + FrontCenter;
        v4 ToBack = Position + BackCenter;
        
        r32 ToFrontDotNormal = Dot(ToFront, Normal);
        r32 ToBackDotNormal = Dot(ToBack, Normal);
        
        ToFrontDotNormal = GSClamp01(ToFrontDotNormal * 1000);
        ToBackDotNormal = GSClamp01(ToBackDotNormal * 1000);
        
        r32 SqDistToCenter = MagSqr(Position);
        if (SqDistToCenter < OuterRadiusSquared && SqDistToCenter > InnerRadiusSquared)
        {
            if (XOR(ToFrontDotNormal > 0, ToBackDotNormal > 0))
            {
                    Assembly->Colors[LED.Index] = Color;
                }
                else
                {
                    Assembly->Colors[LED.Index] = {};
                }
        }
else
                {
                    Assembly->Colors[LED.Index] = {};
                }
        }
    }
}

internal void
TestPatternThree(assembly* Assembly, r32 Time)
{
r32 GreenSize = 20.0f;
    r32 BlueSize = 25.0f;
    r32 RedSize = 25.0f;

    r32 GreenPosition = -GreenSize + (Time * 45);
    r32 BluePosition = -BlueSize + (Time * 25);
    r32 RedPosition = (100 + RedSize) + (Time * -35);
    
    for (s32 Range = 0; Range < Assembly->LEDUniverseMapCount; Range++)
    {
        leds_in_universe_range LEDUniverseRange = Assembly->LEDUniverseMap[Range];
        for (s32 LEDIdx = LEDUniverseRange.RangeStart;
             LEDIdx < LEDUniverseRange.RangeOnePastLast;
             LEDIdx++)
        {
            led LED = Assembly->LEDs[LEDIdx];
            u8 Red = 0;
            u8 Green = 0;
            u8 Blue = 0;
            
            r32 GreenDistance = GSAbs(LED.Position.z - GreenPosition);
            r32 GreenBrightness = GSClamp(0.0f, GreenSize - GreenDistance, GreenSize) / GreenSize;
            Green = (u8)(GreenBrightness * 255);
            
            r32 BlueDistance = GSAbs(LED.Position.z - BluePosition);
            r32 BlueBrightness = GSClamp(0.0f, BlueSize - BlueDistance, BlueSize) / BlueSize;
            Blue = (u8)(BlueBrightness * 255);
            
            r32 RedDistance = GSAbs(LED.Position.z - RedPosition);
            r32 RedBrightness = GSClamp(0.0f, RedSize - RedDistance, RedSize) / RedSize;
            Red = (u8)(RedBrightness * 255);
            
            Assembly->Colors[LED.Index].R = Red;
            Assembly->Colors[LED.Index].B = Blue;
            Assembly->Colors[LED.Index].G = Green;
        }
    }
}
// END TEMPORARY PATTERNS

#include "foldhaus_assembly.cpp"

#include "foldhaus_debug_visuals.h"
//#include "foldhaus_sacn_view.cpp"
#include "foldhaus_text_entry.cpp"
#include "foldhaus_search_lister.cpp"

#include "foldhaus_interface.cpp"
#include "animation/foldhaus_animation_interface.h"

#define PANEL_INIT_PROC(name) void name(panel* Panel)
typedef PANEL_INIT_PROC(panel_init_proc);

#define PANEL_CLEANUP_PROC(name) void name(panel* Panel)
typedef PANEL_CLEANUP_PROC(panel_cleanup_proc);

#define PANEL_RENDER_PROC(name) void name(panel Panel, v2 PanelMin, v2 PanelMax, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
typedef PANEL_RENDER_PROC(panel_render_proc);

#include "panels/foldhaus_panel_sculpture_view.h"
#include "panels/foldhaus_panel_profiler.h"
#include "panels/foldhaus_panel_dmx_view.h"
#include "panels/foldhaus_panel_animation_timeline.h"
#include "panels/foldhaus_panel_hierarchy.h"

#include "generated/foldhaus_panels_generated.h"
