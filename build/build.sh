#!/bin/bash

# --------------------------------------------
#            Usage 

print_usage () {
  echo
  echo Build Command Syntax:
  echo "  $0 [mode] [platform] [arch]"
  echo
  echo "Release Mode Options:"
  echo "  debug"
  echo "  prod"
  echo
  echo "Platform Options:"
  echo "  win32"
  echo "  osx"
  echo "  webgl"
  echo
  echo "Arch Options: (architecture)"
  echo "  intel (valid with Platform Win32 and OSX) (default)"
  echo "  arm64 (only valid for Platform OSX)"
}

# --------------------------------------------
#            Arguments
MODE=$1
PLATFORM=$2
ARCH=$3
PACKAGE=$4

if [ "${MODE}" == "" ] | [ "${PLATFORM}" == "" ]
then
  print_usage
  exit 0
fi

# Default to Intel architecture if none provided
if [ "${ARCH}" == "" ]
then
  ARCH="intel"
fi

if [ "${ARCH}" != "intel" ] && [ "${ARCH}" != "arm64" ]
then
  echo "Uknown target architecture: ${ARCH}"
  print_usage
  exit 0

fi

# --------------------------------------------
#            Utilities

pushdir () {
  command pushd "$@" > /dev/null
}

popdir () {
  command popd "$@" > /dev/null
}

# --------------------------------------------
#         Getting Project Path
#
# Project is stored in PROJECT_PATH

SCRIPT_REL_DIR=$(dirname "${BASH_SOURCE[0]}")
pushdir $SCRIPT_REL_DIR
pushdir ..
PROJECT_PATH=$(pwd)
popdir
popdir

# --------------------------------------------
#         Platform/Mode Specific Variables

# Compiler Selection

Compiler_win32="cl"
Compiler_osx="clang++"
WasiSdk="/c/drive/apps/wasi-sdk"
Compiler_webgl="$WasiSdk/bin/clang++"
Compiler_linux="clang++"

# Platform Entry Points

PlatformEntry_win32="src_v2/platform/win32/lumenarium_first_win32.cpp"
PlatformEntry_osx="src_v2/platform/osx/lumenarium_first_osx.cpp"
PlatformEntry_webgl="src_v2/platform/webgl/lumenarium_first_webgl.cpp"
PlatformEntry_linux="src_v2/platform/linux/lumenarium_first_linux.cpp"

# Intermediate Outputs

CompilerOutput_win32="lumenarium.o"
CompilerOutput_osx="lumenarium"
CompilerOutput_webgl="lumenarium.wasm"
CompilerOutput_linux=""

# Executables

LinkerOutput_win32="lumenarium.exe"
LinkerOutput_osx="lumenarium"
LinkerOutput_webgl="lumenarium.wasm"
LinkerOutput_linux=""

# Wasm Sys Root
WasmSysRoot="${PROJECT_PATH}/src_v2/platform/webgl/sysroot/"

# Compiler Flags

CompilerFlags_win32="-nologo -FC -WX -W4 -Z7 -Oi -MTd -fp:fast"
CompilerFlags_win32="$CompilerFlags_win32 -wd4505 -wd4100 -wd4189"

CompilerFlags_osx=""

CompilerFlags_webgl="-target wasm32 --sysroot ${WasmSysRoot} -s -fvisibility=hidden -fno-builtin -fno-exceptions -fno-threadsafe-statics"

# CompilerFlags_webgl="-nostartfiles -fno-exceptions -fno-entry -strip-all -s -import-memory -fvisibility=hidden --sysroot ${WasmSysRoot}"

CompilerFlags_linux=""

CompilerFlags_DEBUG_win32="-Od -Zi -DDEBUG"
CompilerFlags_DEBUG="-O0 -g -DDEBUG"
CompilerFlags_PROD="-O3"

# Compiler flags that no matter what, we want to define
# for the most part these pass the build parameters into the executable
CompilerFlags_common="-DPLATFORM=${PLATFORM} -DMODE=${MODE} -DARCH=${ARCH}"

# Linker Flags

LinkerFlags_win32="-NOLOGO -incremental:no -subsystem:windows -entry:WinMain -opt:ref"
LinkerFlags_osx=""
# LinkerFlags_webgl="--no-entry --export-dynamic -allow-undefined -import-memory -export=__wasm_call_ctors -export=malloc -export=free -export=main"
LinkerFlags_webgl="--no-entry --export-dynamic --unresolved-symbols=import-functions"
LinkerFlags_linux=""

LinkerFlags_DEBUG="-debug"
LinkerFlags_PROD=""

# Linker Libs

LinkerLibs_win32="user32.lib kernel32.lib gdi32.lib opengl32.lib" 
# winmm.lib gdi32.lib dsound.lib Ws2_32.lib Comdlg32.lib Winspool.lib"
LinkerLibs_osx="-framework OpenGL -framework Cocoa"
LinkerLibs_webgl=""
LinkerLibs_linux=""

# --------------------------------------------
#         Varible Selection

# Select Platform Variables

if [ "${PLATFORM}" == "win32" ]
then
  Compiler=$Compiler_win32
  PlatformEntry=$PlatformEntry_win32
  CompilerFlags=$CompilerFlags_win32
  CompilerOutput=$CompilerOutput_win32
  LinkerOutput=$LinkerOutput_win32
  LinkerFlags=$LinkerFlags_win32
  LinkerLibs=$LinkerLibs_win32

elif [ "${PLATFORM}" == "osx" ]
then
  Compiler=$Compiler_osx
  PlatformEntry=$PlatformEntry_osx
  CompilerFlags=$CompilerFlags_osx
  CompilerOutput=$CompilerOutput_osx
  LinkerOutput=$LinkerOutput_osx
  LinkerFlags=$LinkerFlags_osx
  LinkerLibs=$LinkerLibs_osx

elif [ "${PLATFORM}" == "webgl" ]
then
  Compiler=$Compiler_webgl
  PlatformEntry=$PlatformEntry_webgl
  CompilerFlags=$CompilerFlags_webgl
  CompilerOutput=$CompilerOutput_webgl
  LinkerOutput=$LinkerOutput_webgl
  LinkerFlags=$LinkerFlags_webgl
  LinkerLibs=$LinkerLibs_webgl

elif [ "${PLATFORM}" == "linux" ]
then
  Compiler=$Compiler_linux
  PlatformEntry=$PlatformEntry_linux
  CompilerFlags=$CompilerFlags_linux
  CompilerOutput=$CompilerOutput_linux
  LinkerOutput=$LinkerOutput_linux
  LinkerFlags=$LinkerFlags_linux
  LinkerLibs=$LinkerLibs_linux

else
  echo "Attempting to build for an unknown platform: ${PLATFORM}"
  print_usage
  exit 0

fi

# Select Release Mode Variables

if [ "${MODE}" == "debug" ]
then
  if [ $PLATFORM == "win32" ]
  then
    CompilerFlags="${CompilerFlags} ${CompilerFlags_DEBUG_win32}"
  else
    CompilerFlags="${CompilerFlags} ${CompilerFlags_DEBUG}"
  fi

  LinkerFlags="${LinkerFlags} ${LinkerFlags_DEBUG}"

elif [ "${MODE}" == "prod" ]
then
  CompilerFlags="${CompilerFlags} ${CompilerFlags_PROD}"
  LinkerFlags="${LinkerFlags} ${LinkerFlags_PROD}"

else
  echo "Attempting to build for an unknown release mode: ${MODE}"
  print_usage
  exit 0

fi

# Common Flags
CompilerFlags="${CompilerFlags} ${CompilerFlags_common}"

# --------------------------------------------
#         Build Path Construction
#
# This determines where the generated executable will
# be located. In general, it can be found at
# project_path/run_tree/platform/arch/release_mode/lumenarium.exe
# 
# This section also ensures that the path requested actually exists

BuildDir="${PROJECT_PATH}/run_tree/${PLATFORM}/${ARCH}/${MODE}"
EntryPath="${PROJECT_PATH}/${PlatformEntry}"

# Exception for webgl, which doesn't care about cpu architecture
if [ $PLATFORM == "webgl" ]
then
  BuildDir="${PROJECT_PATH}/run_tree/${PLATFORM}/${MODE}"

fi

# Make the build directory,
#   "-p" flag makes it make the entire tree, and not emit errors if it
#   exists.
mkdir -p "${BuildDir}"

# --------------------------------------------
#         Compilation

echo "Building To: ${BuildDir}/${LinkerOutput}"
echo
pushdir $BuildDir

rm ${CompilerOutput} 2> /dev/null
rm ${LinkerOutput} 2> /dev/null

echo "COMPILING..."
if [ $PLATFORM == "win32" ]
then
  $Compiler $CompilerFlags $EntryPath \
    -link $LinkerFlags $LinkerLibs -OUT:${LinkerOutput}

elif [ $PLATFORM == "webgl" ]
then

  LD="$WasiSdk/bin/wasm-ld"
  echo $Compiler
  CFlags="-O3 -flto --target=wasm32-unknown-wasi"
  LDFlags="--no-entry --export-dynamic --allow-undefined --lto-O3 --import-memory"

  $Compiler \
    -Wno-writable-strings \
    --target=wasm32 \
    -nostdlib \
    -Wl,--export-all \
    -Wl,--no-entry \
    -Wl,--allow-undefined \
    -o lumenarium.wasm \
    $EntryPath \

else
  $Compiler -o $LinkerOutput $CompilerFlags $EntryPath $LinkerLibs

fi

echo "Finished..."
popdir