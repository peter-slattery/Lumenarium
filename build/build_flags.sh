################# COMPILER #################
# Compiler: Global

compiler>input>win32> ${SRC_DIR}/platform/win32/lumenarium_first_win32.c
compiler>input>osx>   ${SRC_DIR}/platform/osx/lumenarium_first_osx.c
compiler>input>raspi> ${SRC_DIR}/platform/raspi/lumenarium_first_raspi.c

msvc>-nologo
msvc>-FC       # full path errors
msvc>-WX       # treat warnings as errors
msvc>-W4       # output warning level
msvc>-Z7       # generate C compatible debug info
# msvc>-Oi     # generate intrinsic functions
# msvc>-MTd    # create a debug multithreaded exe w/ Libcmtd.lib
#msvc>-fp:fast # fast floating point model
msvc>-wd4505   # 
msvc>-wd4100   #
msvc>-wd4189   #
msvc>-wd4702   #
msvc>-wd4996   # _CRT_SECURE_NO_WARNINGS

compiler>wasm>-Wno-writable-strings #
compiler>wasm>--target=wasm32       #
compiler>wasm>-nostdlib             #
compiler>wasm>-Wl,--no-entry        #
compiler>wasm>-Wl,--allow-undefined #
compiler>wasm>-Wl,--export-all      #

compiler>clang>linux>-pthread

compiler>raspi>-pthread
compiler>raspi>-lm      # link with local system math libraries

compiler>clang>arm64>-arch arm64

# Compiler: Debug

compiler>debug>msvc>win32>-Od       #
compiler>debug>msvc>win32>-Zi       #
compiler>debug>msvc>win32>-DDEBUG=1 #
# compiler>debug>msvc>win32>-DPRINT_ASSERTS=1

debug>clang>-O0
debug>clang>-g
debug>compiler>-DDEBUG=1

debug>clang>-fsanitize=address

# Compiler: Prod
compiler>release>clang>-O3

################# LINKER #################

linker>output>osx>lumenarium
linker>output>win32>lumenarium.exe

linker>win32>-NOLOGO
linker>win32>-incremental:no
linker>win32>-subsystem:windows
linker>win32>-opt:ref           # link time optimization - eliminate dead code

linker>wasm>--no-entry
linker>wasm>--export-dynamic
linker>wasm>--unresolved-symbols=import-functions

# TODO: I don't think the build system supports this right now
linker>raspi>-fuse-ld=lld

linker>debug>-debug

################# LIBRARIES #################

linker>libs>win32>user32.lib
linker>libs>win32>kernel32.lib
linker>libs>win32>gdi32.lib
linker>libs>win32>opengl32.lib
linker>libs>win32>winmm.lib
linker>libs>win32>gdi32.lib
linker>libs>win32>dsound.lib
linker>libs>win32>Ws2_32.lib
linker>libs>win32>Comdlg32.lib
linker>libs>win32>Winspool.lib

linker>libs>osx>-framework OpenGL
linker>libs>osx>-framework Cocoa
linker>libs>osx>-framework IOKit
linker>libs>osx>${SRC_DIR}/libs/glfw_osx/lib-universal/libglfw3.a
