#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "../linux/lumenarium_linux_memory.h"
#include "../../core/lumenarium_core.h"
#include "../lumenarium_os.h"
#include "../../lumenarium_first.c"

#undef internal
#undef external

#include <errno.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define linux_err_print(sub_proc) linux_err_print_((char*)__FUNCTION__, (char*)(sub_proc), errno)
void 
linux_err_print_(char* proc, char* sub_proc, s32 errsv)
{
  printf("Error: %s:%s - %d\n\t%s\n\n", proc, sub_proc, errsv, strerror(errsv));
}

#define OS_FILE_HANDLE_TYPE s32
#define OS_FILE_MAX_PATH PATH_MAX
#define OS_FILE_INVALID_HANDLE -1
#define OS_SOCKET_TYPE s32
#define OS_SOCKET_INVALID_HANDLE -1
#include "../shared/lumenarium_shared_file_tracker.h"
#include "../shared/lumenarium_shared_file_async_work_on_job.h"
#include "../shared/lumenarium_shared_network.h"
#include "../linux/lumenarium_linux_file.h"
#include "../linux/lumenarium_linux_time.h"
#include "../linux/lumenarium_linux_network.h"


int main (int arg_count, char** args)
{
  // INIT APPLICATION
  Editor_Desc ed_desc = {
  };
  App_State* state = lumenarium_init(&ed_desc);

  bool running = true;
  r64 target_seconds_per_frame = 1.0 / 30.0;
  Ticks ticks_start = os_get_ticks();
  while (has_flag(state->flags, AppState_IsRunning))
  {
    lumenarium_frame_prepare(state);

    lumenarium_frame(state);
    lumenarium_env_validate();

    Ticks ticks_end = os_get_ticks();
    r64 seconds_elapsed = get_seconds_elapsed(ticks_start, ticks_end, os_get_ticks_per_second());
    while (seconds_elapsed < target_seconds_per_frame)
    {
      u32 sleep_time = (u32)(1000.0f * (target_seconds_per_frame - seconds_elapsed));
      usleep(sleep_time);
      ticks_end = os_get_ticks();
      seconds_elapsed = get_seconds_elapsed(ticks_start, ticks_end, os_get_ticks_per_second());
    }
    ticks_start = ticks_end;
  }

  lumenarium_cleanup(state);
  return 0;
}