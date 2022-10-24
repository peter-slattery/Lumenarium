// NOTE(PS):
// need to include before this:
//    #include <stdarg.h>
//    #include <stdint.h>
//    #include <math.h>

#if defined(__clang__)
# pragma GCC diagnostic ignored "-Wunused-value"
# pragma GCC diagnostic ignored "-Wvarargs"
# pragma GCC diagnostic ignored "-Wwritable-strings"
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#if defined(DEBUG)
# define USE_ASSERTS 1
#endif

#include "lumenarium_core_assert.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION
#define STBTT_assert(x) assert(x)
#include "../libs/stb_truetype.h"
#include "../libs/stb_sprintf.h"

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_STATIC
#include "../libs/HandmadeMath.h"
typedef hmm_v2 v2;
typedef hmm_v3 v3;
typedef hmm_v4 v4;
typedef hmm_mat4 m44;

#include "lumenarium_core_types.h"
#include "lumenarium_core_memory.h"
#include "lumenarium_core_string.h"
#include "lumenarium_core_hash.h"
#include "lumenarium_core_random.h"

#include "lumenarium_core_file.h"
#include "lumenarium_core_window.h"
#include "lumenarium_core_time.h"
#include "lumenarium_core_threads.h"
#include "lumenarium_core_socket.h"