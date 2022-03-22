/* date = March 22nd 2022 2:29 am */

#ifndef LUMENARIUM_FIRST_H
#define LUMENARIUM_FIRST_H

typedef struct App_State App_State;

#include "lumenarium_memory.cpp"
#include "lumenarium_string.cpp"
#include "lumenarium_input.cpp"

#include "engine/lumenarium_engine_assembly.h"
#include "engine/lumenarium_engine.cpp"

#include "editor/lumenarium_editor_renderer.cpp"
#include "editor/lumenarium_editor.cpp"

//////////////////////////////////////////////
//    Lumenarium Runtime Environment

global Allocator* permanent;
global Allocator* scratch; // gets reset at frame boundaries

#if defined(DEBUG)
#  include "lumenarium_tests.cpp"
#else
#  define run_tests()
#endif

//////////////////////////////////////////////
//    Lumenarium State

typedef b32 App_State_Flags;
enum
{
  AppState_None = 0,
  AppState_IsRunning = 1,
  AppState_NoEditor = 2,
};

struct App_State
{
  App_State_Flags flags;
  
  Input_State input_state;
};

#endif //LUMENARIUM_FIRST_H
