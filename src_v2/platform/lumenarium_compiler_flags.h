/* date = March 22nd 2022 2:09 am */

#ifndef LUMENARIUM_COMPILER_FLAGS_H
#define LUMENARIUM_COMPILER_FLAGS_H

#if defined(__clang__)
# pragma GCC diagnostic ignored "-Wunused-value"
# pragma GCC diagnostic ignored "-Wvarargs"
# pragma GCC diagnostic ignored "-Wwritable-strings"
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef DEBUG
# define USE_ASSERTS 1
#endif

#endif //LUMENARIUM_COMPILER_FLAGS_H
