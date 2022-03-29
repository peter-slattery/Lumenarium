/* date = March 22nd 2022 2:12 am */

#ifndef LUMENARIUM_PLATFORM_COMMON_INCLUDES_H
#define LUMENARIUM_PLATFORM_COMMON_INCLUDES_H

#include <stdarg.h>

#if !defined(GUESS_INTS)
# include <stdint.h>
#endif // !defined(GUESS_INTS)

#include <math.h>

#include "lumenarium_assert.h"

// NOTE(PS): only need the opengl extension headers
// when running on a platform that is using opengl 3.3+
#if !defined(PLATFORM_wasm)
#  include "glcorearb.h"
#  include "glext.h"
#  include "wglext.h"
#endif

#if 0
#define HMM_SINF sin
#define HMM_COSF cos
#define HMM_TANF tan
#define HMM_SQRTF sqrt
#define HMM_EXPF exp
#define HMM_LOGF log
#define HMM_ACOSF acos
#define HMM_ATANF atan
#define HMM_ATAN2F atan2
#endif

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_STATIC
#include "../libs/HandmadeMath.h"

typedef hmm_v2 v2;
typedef hmm_v3 v3;
typedef hmm_v4 v4;
typedef hmm_mat4 m44;

#endif //LUMENARIUM_PLATFORM_COMMON_INCLUDES_H
