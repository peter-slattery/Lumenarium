/* date = March 22nd 2022 2:29 am */

#ifndef LUMENARIUM_FIRST_H
#define LUMENARIUM_FIRST_H

typedef struct App_State App_State;

// Environment
#include "lumenarium_texture_atlas.c"
#include "lumenarium_geometry.h"

global Allocator* global_scratch_; // gets reset at frame boundaries
// TODO make sure all scratch_get's have a release
#define scratch_get(ident) Allocator_Scratch ident = allocator_scratch_begin(global_scratch_)
#define scratch_release(ident) allocator_scratch_end(&ident)
#include "lumenarium_bsp.h"

#include "patterns/patterns_math.h"

// Engine
typedef struct Assembly_Strip Assembly_Strip;
typedef struct Assembly_Pixel_Buffer Assembly_Pixel_Buffer;
#include "engine/lumenarium_engine_output.h"
#include "engine/lumenarium_engine_assembly.h"
#include "engine/output/lumenarium_output_uart.h"
#include "engine/output/lumenarium_output_sacn.h"

// Editor
#if defined(PLATFORM_SUPPORTS_EDITOR)
#  include "editor/graphics/lumenarium_editor_opengl.h"
#  include "editor/graphics/lumenarium_editor_graphics.h"
#  include "editor/lumenarium_editor_ui.h"
#  include "editor/lumenarium_editor_renderer.h"
#  include "editor/lumenarium_editor.h"
#endif // PLATFORM_SUPPORTS_EDITOR

//////////////////////////////////////////////
//    Lumenarium Runtime Environment

global Allocator* permanent;

#if defined(DEBUG)
#  include "lumenarium_tests.cpp"
#define lumenarium_env_validate() lumenarium_env_validate_()
#else
#  define run_tests()
#define lumenarium_env_validate()
#endif

internal void
lumenarium_env_validate_()
{
  bump_allocator_validate(permanent);
  bump_allocator_validate(global_scratch_);
  bump_allocator_validate(file_jobs_arena);

  core_socket_tests();
}



//////////////////////////////////////////////
//    Lumenarium State

typedef b32 App_State_Flags;
enum
{
  AppState_None = 0,
  AppState_IsRunning = 1,
  AppState_RunEditor = 2,
  AppState_RunUserSpace = 4,
};

typedef struct App_Init_Desc App_Init_Desc;
struct App_Init_Desc
{
  u32 assembly_cap;
};

typedef struct App_State App_State;
struct App_State
{
  App_State_Flags flags;
  File_Async_Job_System file_async_job_system;
  
  Input_State* input_state;
  
  Assembly_Array assemblies;
  Output output;
  
  #if defined(PLATFORM_SUPPORTS_EDITOR)
    Editor* editor;
  #endif
};

typedef struct Editor_Desc Editor_Desc;
struct Editor_Desc
{
  v2 content_scale;
  v2 init_window_dim;
};

void sculpture_updated(App_State* state, r32 scale, r32 led_size);

#include "user_space/user_space_incenter.h"
#include "../run_tree/data/incenter_test_data.c"

#include "engine/lumenarium_engine_assembly.c"
#include "engine/lumenarium_engine.c"
#include "engine/lumenarium_engine_output.c"
#include "engine/output/lumenarium_output_uart.c"
#include "engine/output/lumenarium_output_sacn.c"

#if defined(PLATFORM_SUPPORTS_EDITOR)
#  include "editor/lumenarium_editor_ui.c"
#  include "editor/lumenarium_editor_renderer.c"
#  include "editor/lumenarium_editor_sculpture_visualizer.c"
#  include "editor/lumenarium_editor.c"
#endif

#include "tools/convert_csv.c"

#endif //LUMENARIUM_FIRST_H
