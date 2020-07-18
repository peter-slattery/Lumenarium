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
#include "assembly_parser.cpp"

typedef struct app_state app_state;

// TODO(Peter): something we can do later is to remove all reliance on app_state and context
// from foldhaus_pane.h. It should just emit lists of things that the app can iterate over and
// perform operations on, like panel_draw_requests = { bounds, panel* } etc.
#include "foldhaus_panel.h"

#include "foldhaus_command_dispatch.h"
#include "foldhaus_operation_mode.h"

#include "animation/foldhaus_animation.h"

enum network_protocol
{
    NetworkProtocol_SACN,
    NetworkProtocol_ArtNet,
    
    NetworkProtocol_Count,
};

struct app_state
{
    r32 CameraTheta; // TODO(Peter): @TEMPORARY
    rect2 WindowBounds;
    
    gs_memory_arena Permanent;
    gs_memory_arena Transient;
    
    s32 NetworkProtocolHeaderSize;
    network_protocol NetworkProtocol;
    
    streaming_acn SACN;
    
    led_system LedSystem;
    assembly_array Assemblies;
    
    camera Camera;
    r32 PixelsToWorldScale;
    
    operation_mode_system Modes;
    input_command_queue CommandQueue;
    
    ui_interface Interface;
    
    animation_system AnimationSystem;
    gs_list_handle SelectedAnimationBlockHandle;
    u32 SelectedAnimationLayer;
    
    panel_system PanelSystem;
    panel* HotPanel;
    
    //pattern_node_workspace NodeWorkspace;
    
    event_log* GlobalLog;
};

internal void OpenColorPicker(app_state* State, v4* Address);

// BEGIN TEMPORARY PATTERNS
internal void
TestPatternOne(led_buffer* Assembly, r32 Time)
{
    for (u32 LedIndex = 0; LedIndex < Assembly->LedCount; LedIndex++)
    {
        v4 LedPosition = Assembly->Positions[LedIndex];
        float PercentX = RemapClampedR32(LedPosition.x, -150.0f, 150.0f, 0.0f, 1.0f);
        float PercentY = RemapClampedR32(LedPosition.y, -150.0f, 150.0f, 0.0f, 1.0f);
        Assembly->Colors[LedIndex].R = (u8)(PercentX * 255);
        Assembly->Colors[LedIndex].G = (u8)(PercentY * 255);
    }
}

internal void
TestPatternTwo(led_buffer* Assembly, r32 Time)
{
    r32 PeriodicTime = (Time / PiR32) * 2;
    
    r32 ZeroOneSin = (SinR32(PeriodicTime) * .5f) + .5f;
    r32 ZeroOneCos = (CosR32(PeriodicTime) * .5f) + .5f;
    pixel Color = { (u8)(ZeroOneSin * 255), 0, (u8)(ZeroOneCos * 255) };
    
    v4 Center = v4{0, 0, 0, 1};
    r32 ThetaZ = Time / 2;
    v4 Normal = v4{CosR32(ThetaZ), 0, SinR32(ThetaZ), 0}; // NOTE(Peter): dont' need to normalize. Should always be 1
    v4 Right = V4Cross(Normal, v4{0, 1, 0, 0});
    
    v4 FrontCenter = Center + (Normal * 25);
    v4 BackCenter = Center - (Normal * 25);
    
    r32 OuterRadiusSquared = 1000000;
    r32 InnerRadiusSquared = 0;
    
    for (u32 LedIndex = 0; LedIndex < Assembly->LedCount; LedIndex++)
    {
        v4 Position = Assembly->Positions[LedIndex];
        
        v4 ToFront = Position + FrontCenter;
        v4 ToBack = Position + BackCenter;
        
        r32 ToFrontDotNormal = V4Dot(ToFront, Normal);
        r32 ToBackDotNormal = V4Dot(ToBack, Normal);
        
        ToFrontDotNormal = Clamp01(ToFrontDotNormal * 1000);
        ToBackDotNormal = Clamp01(ToBackDotNormal * 1000);
        
        r32 SqDistToCenter = V4MagSquared(Position);
        if (SqDistToCenter < OuterRadiusSquared && SqDistToCenter > InnerRadiusSquared)
        {
            if (XOR(ToFrontDotNormal > 0, ToBackDotNormal > 0))
            {
                Assembly->Colors[LedIndex] = Color;
            }
            else
            {
                //Assembly->Colors[LedIndex] = {};
            }
        }
        else
        {
            //Assembly->Colors[LedIndex] = {};
        }
    }
}

internal void
TestPatternThree(led_buffer* Assembly, r32 Time)
{
    v4 GreenCenter = v4{0, 0, 150, 1};
    r32 GreenRadius = Abs(SinR32(Time)) * 200;
    
    v4 TealCenter = v4{0, 0, 150, 1};
    r32 TealRadius = Abs(SinR32(Time + 1.5)) * 200;
    
    r32 FadeDist = 35;
    
    
    for (u32 LedIndex = 0; LedIndex < Assembly->LedCount; LedIndex++)
    {
        v4 LedPosition = Assembly->Positions[LedIndex];
        u8 Red = 0;
        u8 Green = 0;
        u8 Blue = 0;
        
        r32 GreenDist = Abs(V4Mag(LedPosition - GreenCenter) - GreenRadius);
        r32 GreenBrightness = Clamp(0.f, FadeDist - Abs(GreenDist), FadeDist);
        Green = (u8)(GreenBrightness * 255);
        
        r32 TealDist = Abs(V4Mag(LedPosition - TealCenter) - TealRadius);
        r32 TealBrightness = Clamp(0.f, FadeDist - Abs(TealDist), FadeDist);
        Red = (u8)(TealBrightness * 255);
        Blue = (u8)(TealBrightness * 255);
        
        Assembly->Colors[LedIndex].R = Red;
        Assembly->Colors[LedIndex].B = Green;
        Assembly->Colors[LedIndex].G = Green;
    }
}

// END TEMPORARY PATTERNS

#include "foldhaus_assembly.cpp"

FOLDHAUS_INPUT_COMMAND_PROC(EndCurrentOperationMode)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

#define PANEL_INIT_PROC(name) void name(panel* Panel, app_state* State)
typedef PANEL_INIT_PROC(panel_init_proc);

#define PANEL_CLEANUP_PROC(name) void name(panel* Panel, app_state* State)
typedef PANEL_CLEANUP_PROC(panel_cleanup_proc);

#define PANEL_RENDER_PROC(name) void name(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
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
#include "panels/foldhaus_panel_file_view.h"

#include "generated/foldhaus_panels_generated.h"

#include "foldhaus_interface.cpp"

#include "../meta/gs_meta_include.cpp"

#define FOLDHAUS_APP_H
#endif // FOLDHAUS_APP_H