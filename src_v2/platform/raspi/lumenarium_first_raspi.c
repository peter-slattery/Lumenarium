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
  // temp
  global_scratch_ = bump_allocator_create_reserve(MB(256));
  run_tests();

  return 0;
}