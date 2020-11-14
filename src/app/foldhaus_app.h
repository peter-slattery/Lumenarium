//
// File: foldhaus_app.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_APP_H

#include "../meta/gs_meta_include.h"
#include "../meta/gs_meta_lexer.h"

#include "engine/foldhaus_serializer.h"

#include "../gs_libs/gs_font.h"
#include "foldhaus_log.h"

#include "interface.h"

#include "engine/foldhaus_network_ordering.h"

#include "engine/assembly/foldhaus_assembly.h"
#include "engine/assembly/foldhaus_assembly_parser.cpp"

#include "engine/sacn/foldhaus_sacn.h"
#include "engine/uart/foldhaus_uart.h"
#include "engine/uart/foldhaus_uart.cpp"

typedef struct app_state app_state;

typedef struct panel panel;

#include "editor/foldhaus_command_dispatch.h"
#include "editor/foldhaus_operation_mode.h"

// TODO(Peter): something we can do later is to remove all reliance on app_state and context
// from foldhaus_pane.h. It should just emit lists of things that the app can iterate over and
// perform operations on, like panel_draw_requests = { bounds, panel* } etc.
#include "editor/foldhaus_panel.h"

#include "engine/animation/foldhaus_animation.h"
#include "engine/animation/foldhaus_animation_serializer.cpp"

struct app_state
{
    gs_memory_arena Permanent;
    gs_memory_arena* Transient;
    
    // Engine
    //
    network_protocol NetworkProtocol;
    streaming_acn SACN;
    led_system LedSystem;
    assembly_array Assemblies;
    animation_system AnimationSystem;
    event_log* GlobalLog;
    
    // Interface
    //
    rect2 WindowBounds;
    
    operation_mode_system Modes;
    input_command_queue CommandQueue;
    
    ui_interface Interface;
    panel_system PanelSystem;
    panel* HotPanel;
    
    r32 PixelsToWorldScale;
};

internal void OpenColorPicker(app_state* State, v4* Address);

#include "engine/assembly/foldhaus_assembly.cpp"

// BEGIN TEMPORARY PATTERNS
internal void
TestPatternOne(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient)
{
    led_strip_list BlumenStrips = AssemblyStripsGetWithTagValue(Assembly, ConstString("assembly"), ConstString("Blumen Lumen"), Transient);
    led_strip_list RadiaStrips = AssemblyStripsGetWithTagValue(Assembly, ConstString("assembly"), ConstString("Radialumia"), Transient);
    
    for (u32 i = 0; i < BlumenStrips.Count; i++)
    {
        u32 StripIndex = BlumenStrips.StripIndices[i];
        v2_strip StripAt = Assembly.Strips[StripIndex];
        
        for (u32 j = 0; j < StripAt.LedCount; j++)
        {
            u32 LedIndex = StripAt.LedLUT[j];
            Leds->Colors[LedIndex] = { 255, 0, 0 };
            
        }
    }
    
    for (u32 i = 0; i < RadiaStrips.Count; i++)
    {
        u32 StripIndex = RadiaStrips.StripIndices[i];
        v2_strip StripAt = Assembly.Strips[StripIndex];
        
        for (u32 j = 0; j < StripAt.LedCount; j++)
        {
            u32 LedIndex = StripAt.LedLUT[j];
            Leds->Colors[LedIndex] = { 0, 255, 0 };
        }
    }
#if 0
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 LedPosition = Leds->Positions[LedIndex];
        float PercentX = RemapClampedR32(LedPosition.x, -150.0f, 150.0f, 0.0f, 1.0f);
        float PercentY = RemapClampedR32(LedPosition.y, -150.0f, 150.0f, 0.0f, 1.0f);
        Leds->Colors[LedIndex].R = (u8)(PercentX * 255);
        Leds->Colors[LedIndex].G = (u8)(PercentY * 255);
    }
#endif
    
}

internal void
TestPatternTwo(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient)
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
    
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 Position = Leds->Positions[LedIndex];
        
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
                Leds->Colors[LedIndex] = Color;
            }
            else
            {
                //Leds->Colors[LedIndex] = {};
            }
        }
        else
        {
            //Leds->Colors[LedIndex] = {};
        }
    }
}

internal void
TestPatternThree(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient)
{
    v4 GreenCenter = v4{0, 0, 150, 1};
    r32 GreenRadius = Abs(SinR32(Time)) * 200;
    
    v4 TealCenter = v4{0, 0, 150, 1};
    r32 TealRadius = Abs(SinR32(Time + 1.5)) * 200;
    
    r32 FadeDist = 35;
    
    
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 LedPosition = Leds->Positions[LedIndex];
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
        
        Leds->Colors[LedIndex].R = Red;
        Leds->Colors[LedIndex].B = Green;
        Leds->Colors[LedIndex].G = Green;
    }
}

// END TEMPORARY PATTERNS

internal void
EndCurrentOperationMode(app_state* State)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

s32 GlobalAnimationPatternsCount = 3;
animation_pattern GlobalAnimationPatterns[] = {
    { "Test Pattern One", 16, TestPatternOne  },
    { "Test Pattern Two", 16, TestPatternTwo },
    { "Test Pattern Three", 18, TestPatternThree },
};

#include "editor/panels/foldhaus_panel_types.h"

#include "editor/panels/foldhaus_panel_file_view.h"
#include "editor/panels/foldhaus_panel_sculpture_view.h"
#include "editor/panels/foldhaus_panel_profiler.h"
#include "editor/panels/foldhaus_panel_dmx_view.h"
#include "editor/panels/foldhaus_panel_animation_timeline.h"
#include "editor/panels/foldhaus_panel_hierarchy.h"


#include "editor/panels/foldhaus_panel_types.cpp"
//#include "generated/foldhaus_panels_generated.h"

#include "editor/foldhaus_interface.cpp"

#include "../meta/gs_meta_include.cpp"

#include "editor/foldhaus_editor.cpp"

#define FOLDHAUS_APP_H
#endif // FOLDHAUS_APP_H