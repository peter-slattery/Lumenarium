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

#include "editor/interface.h"

#include "engine/foldhaus_network_ordering.h"

#include "engine/assembly/foldhaus_assembly.h"

#include "ss_blumen_lumen/gfx_math.h"

#include "engine/assembly/foldhaus_assembly_parser.cpp"
#include "engine/assembly/foldhaus_assembly_debug.h"

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
#include "engine/animation/foldhaus_animation_renderer.cpp"

#include "engine/user_space.h"
#include "ss_blumen_lumen/phrase_hue_map.h"
#include "ss_blumen_lumen/blumen_lumen.h"

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
    assembly_debug_state AssemblyDebugState;
    animation_system AnimationSystem;
    animation_pattern_array Patterns;
    
    // Interface
    //
    rect2 WindowBounds;
    
    operation_mode_system Modes;
    input_command_queue CommandQueue;
    
    ui_interface Interface;
    panel_system PanelSystem;
    panel* HotPanel;
    
    user_space_desc UserSpaceDesc;
    bool ShowingUserSpaceDebug;
    
    bool RunEditor;
    bool SendEmptyPackets;
};

internal void OpenColorPicker(app_state* State, v4* Address);

#include "engine/assembly/foldhaus_assembly.cpp"

internal assembly*
LoadAssembly(gs_const_string Path, app_state* State, context Context)
{
    return LoadAssembly(&State->Assemblies, 
                        &State->LedSystem, 
                        State->Transient, 
                        Context, 
                        Path,
                        GlobalLogBuffer);
}

#include "engine/user_space.cpp"

#include "ss_blumen_lumen/sdf.h"
#include "patterns/blumen_patterns.h"
#include "ss_blumen_lumen/blumen_lumen.cpp"

internal void
EndCurrentOperationMode(app_state* State)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

#include "editor/panels/foldhaus_panel_types.h"

#include "editor/panels/foldhaus_panel_file_view.h"
#include "editor/panels/foldhaus_panel_sculpture_view.h"
#include "editor/panels/foldhaus_panel_profiler.h"
#include "editor/panels/foldhaus_panel_dmx_view.h"
#include "editor/panels/foldhaus_panel_animation_timeline.h"
#include "editor/panels/foldhaus_panel_hierarchy.h"
#include "editor/panels/foldhaus_panel_assembly_debug.h"
#include "editor/panels/foldhaus_panel_message_log.h"

#include "editor/panels/foldhaus_panel_types.cpp"

#include "editor/foldhaus_interface.cpp"
#include "editor/foldhaus_editor_draw.h"
#include "editor/foldhaus_editor.cpp"

#define FOLDHAUS_APP_H
#endif // FOLDHAUS_APP_H