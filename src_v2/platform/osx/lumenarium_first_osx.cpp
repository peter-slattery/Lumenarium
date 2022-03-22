
#include "../lumenarium_compiler_flags.h"
#include "../lumenarium_platform_common_includes.h"

#include "../../lumenarium_types.h"
#include "../lumenarium_platform.h"
#include "../../lumenarium_first.cpp"

#include <stdio.h>
#include <stdlib.h>

#include "lumenarium_osx_memory.cpp"

int main (int arg_count, char** args)
{
  App_State* state = lumenarium_init();
  
  while (has_flag(state->flags, AppState_IsRunning))
  {
    // TODO(PS): event processing
    
    lumenarium_update(state);
  }
  
  lumenarium_cleanup(state);
  
  return 0;
}