//
// File: foldhaus_app.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_APP_H

#include "../meta/gs_meta_include.h"
#include "../meta/gs_meta_lexer.h"

#include "../gs_libs/gs_font.h"
#include "foldhaus_log.h"

#include "interface.h"

#include "foldhaus_network_ordering.h"
#include "dmx/dmx.h"
#include "sacn/sacn.h"

#include "foldhaus_assembly.h"

#include "assembly_parser.h"

#include "foldhaus_node.h"

// TODO(Peter): TEMPORARY
//u32 NodeSpecificationsCount = 0;
//node_specification* NodeSpecifications = 0;


#include "assembly_parser.cpp"
#include "test_patterns.h"

// TODO(Peter): something we can do later is to remove all reliance on app_state and context
// from foldhaus_pane.h. It should just emit lists of things that the app can iterate over and
// perform operations on, like panel_draw_requests = { bounds, panel* } etc.
#include "foldhaus_panel.h"

typedef struct app_state app_state;

#include "foldhaus_command_dispatch.h"
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
    rect WindowBounds;
    
    memory_arena Permanent; 
    memory_arena Transient;
    
    s32 NetworkProtocolHeaderSize;
    network_protocol NetworkProtocol;
    
    streaming_acn SACN;
    
    s32 TotalLEDsCount;
    gs_list<assembly> AssemblyList;
    gs_list<gs_list_handle> ActiveAssemblyIndecies;
    
    camera Camera;
    r32 PixelsToWorldScale;
    
    operation_mode_system Modes;
    input_command_queue CommandQueue;
    text_entry ActiveTextEntry;
    
    interface_config Interface;
    
    animation_system AnimationSystem;
    gs_list_handle SelectedAnimationBlockHandle;
    u32 SelectedAnimationLayer;
    
    panel_system PanelSystem;
    panel* HotPanel;
    
    pattern_node_workspace NodeWorkspace;
    
    event_log* GlobalLog;
};

internal void OpenColorPicker(app_state* State, v4* Address);

// BEGIN TEMPORARY PATTERNS
internal void
TestPatternOne(assembly_led_buffer* Assembly, r32 Time)
{
    for (u32 LEDIdx = 0; LEDIdx < Assembly->LEDCount; LEDIdx++)
    {
        led LED = Assembly->LEDs[LEDIdx];
        if (LED.Position.x < 0)
        {
            Assembly->Colors[LED.Index].R = 255;
            Assembly->Colors[LED.Index].B = 255;
            Assembly->Colors[LED.Index].G = 255;
        }
        else
        {
            Assembly->Colors[LED.Index].R = 0;
            Assembly->Colors[LED.Index].B = 0;
            Assembly->Colors[LED.Index].G = 0;
        }
    }
}

internal void
TestPatternTwo(assembly_led_buffer* Assembly, r32 Time)
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
    
    for (u32 LEDIdx = 0; LEDIdx < Assembly->LEDCount; LEDIdx++)
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
                //Assembly->Colors[LED.Index] = {};
            }
        }
        else
        {
            //Assembly->Colors[LED.Index] = {};
        }
    }
}

internal void
TestPatternThree(assembly_led_buffer* Assembly, r32 Time)
{
    v4 GreenCenter = v4{0, 0, 150, 1};
    r32 GreenRadius = GSAbs(GSSin(Time)) * 200;
    
    v4 TealCenter = v4{0, 0, 150, 1};
    r32 TealRadius = GSAbs(GSSin(Time + 1.5)) * 200;
    
    r32 FadeDist = 35;
    
    
    for (u32 LEDIdx = 0; LEDIdx < Assembly->LEDCount; LEDIdx++)
    {
        led LED = Assembly->LEDs[LEDIdx];
        u8 Red = 0;
        u8 Green = 0;
        u8 Blue = 0;
        
        r32 GreenDist = GSAbs(Mag(LED.Position - GreenCenter) - GreenRadius);
        r32 GreenBrightness = GSClamp(0.f, FadeDist - GSAbs(GreenDist), FadeDist);
        Green = (u8)(GreenBrightness * 255);
        
        r32 TealDist = GSAbs(Mag(LED.Position - TealCenter) - TealRadius);
        r32 TealBrightness = GSClamp(0.f, FadeDist - GSAbs(TealDist), FadeDist);
        Red = (u8)(TealBrightness * 255);
        Blue = (u8)(TealBrightness * 255);
        
        Assembly->Colors[LED.Index].R = Red;
        Assembly->Colors[LED.Index].B = Green;
        Assembly->Colors[LED.Index].G = Green;
    }
}

// END TEMPORARY PATTERNS

#include "foldhaus_assembly.cpp"

#include "foldhaus_text_entry.cpp"
#include "foldhaus_search_lister.cpp"

#include "foldhaus_default_nodes.h"

#include "./generated/gs_meta_generated_typeinfo.h"
#include "generated/foldhaus_nodes_generated.h"


#include "foldhaus_node.cpp"

FOLDHAUS_INPUT_COMMAND_PROC(EndCurrentOperationMode)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

#define PANEL_INIT_PROC(name) void name(panel* Panel, app_state* State)
typedef PANEL_INIT_PROC(panel_init_proc);

#define PANEL_CLEANUP_PROC(name) void name(panel* Panel, app_state* State)
typedef PANEL_CLEANUP_PROC(panel_cleanup_proc);

#define PANEL_RENDER_PROC(name) void name(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
typedef PANEL_RENDER_PROC(panel_render_proc);

// NOTE(Peter): This is used by the meta system to generate panel type info
struct panel_definition
{
    char* PanelName;
    s32 PanelNameLength;
    panel_init_proc* Init;
    panel_cleanup_proc* Cleanup;
    panel_render_proc* Render;
    input_command* InputCommands;
    s32 InputCommandsCount;
};

#include "panels/foldhaus_panel_sculpture_view.h"
#include "panels/foldhaus_panel_profiler.h"
#include "panels/foldhaus_panel_dmx_view.h"
#include "panels/foldhaus_panel_animation_timeline.h"
#include "panels/foldhaus_panel_hierarchy.h"
#include "panels/foldhaus_panel_node_graph.h"
#include "panels/foldhaus_panel_file_view.h"

#include "generated/foldhaus_panels_generated.h"

#include "foldhaus_interface.cpp"

#include "../meta/gs_meta_include.cpp"

#define FOLDHAUS_APP_H
#endif // FOLDHAUS_APP_H