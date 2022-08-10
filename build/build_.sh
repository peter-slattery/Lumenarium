#!/bin/bash

# Ensure an error makes the script bail
set -e

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
  echo "  wasm"
  echo "  raspi"
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

add_flag () {
  local -n ref=$1
  ref="$ref $2"
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
Compiler_osx="clang"
Compiler_raspi="clang"
WasiSdk="/c/drive/apps/wasi-sdk"
Compiler_wasm="$WasiSdk/bin/clang++"
Compiler_linux="clang++"

# Platform Entry Points

PlatformEntry_win32="src_v2/platform/win32/lumenarium_first_win32.cpp"
PlatformEntry_osx="src_v2/platform/osx/lumenarium_first_osx.c"
PlatformEntry_wasm="src_v2/platform/wasm/lumenarium_first_wasm.cpp"
PlatformEntry_linux="src_v2/platform/linux/lumenarium_first_linux.cpp"
PlatformEntry_raspi="src_v2/platform/raspi/lumenarium_first_raspi.c"

# Intermediate Outputs

CompilerOutput_win32="lumenarium.o"
CompilerOutput_osx="lumenarium"
CompilerOutput_wasm="lumenarium.wasm"
CompilerOutput_linux=""
CompilerOutput_raspi="lumenarium"

# Executables

LinkerOutput_win32="lumenarium.exe"
LinkerOutput_osx="lumenarium"
LinkerOutput_wasm="lumenarium.wasm"
LinkerOutput_linux=""
LinkerOutput_raspi="lumenarium"

# Wasm Sys Root
WasmSysRoot="${PROJECT_PATH}/src_v2/platform/wasm/sysroot/"

# Compiler Flags

CompilerFlags_win32="-nologo"
CompilerFlags_win32+=" -FC" # display errors with full path
CompilerFlags_win32+=" -WX" # treat warnings as errors
CompilerFlags_win32+=" -W4" # output warning level
CompilerFlags_win32+=" -Z7" # generate C compatible debug info
# CompilerFlags_win32+="-Oi" # generate intrinsic functions
# CompilerFlags_win32+="-MTd" # create a debug multithreaded exe w/ Libcmtd.lib
# CompilerFlags_win32+="-fp:fast" # fast floating point model
CompilerFlags_win32+=" -wd4505" # 
CompilerFlags_win32+=" -wd4100" #
CompilerFlags_win32+=" -wd4189" #
CompilerFlags_win32+=" -wd4702" #
CompilerFlags_win32+=" -wd4996" # _CRT_SECURE_NO_WARNINGS

CompilerFlags_osx=""

CompilerFlags_wasm=""
CompilerFlags_wasm+=" -Wno-writable-strings" #
CompilerFlags_wasm+=" --target=wasm32" #
CompilerFlags_wasm+=" -nostdlib" #
CompilerFlags_wasm+=" -Wl,--no-entry" #
CompilerFlags_wasm+=" -Wl,--allow-undefined" #
CompilerFlags_wasm+=" -Wl,--export-all" #

CompilerFlags_linux=""

CompilerFlags_raspi="" # "--target=arm-rpi-linux-gnueabihf" # "--target=arm-linux-gnueabihf" #target
CompilerFlags_raspi+="-lm" # link with local system math libraries


CompilerFlags_DEBUG_win32=""
CompilerFlags_DEBUG_win32+=" -Od" #
CompilerFlags_DEBUG_win32+=" -Zi" #
CompilerFlags_DEBUG_win32+=" -DDEBUG" #
# add_flag CompilerFlags_DEBUG_win32 "-DPRINT_ASSERTS"

CompilerFlags_DEBUG="-O0"
CompilerFlags_DEBUG+=" -g" #
CompilerFlags_DEBUG+=" -DDEBUG" #
if [ "${PLATFORM}" != "raspi" ]
then
  CompilerFlags_DEBUG+=" -fsanitize=address" #address sanitizer
fi

CompilerFlags_PROD=" -O3"

# Compiler flags that no matter what, we want to define
# for the most part these pass the build parameters into the executable
CompilerFlags_common=" -DPLATFORM_${PLATFORM}=1 -DMODE_${MODE}=1 -DARCH_${ARCH}=1"

# Linker Flags

LinkerFlags_win32=" -NOLOGO"
LinkerFlags_win32+=" -incremental:no" #
LinkerFlags_win32+=" -subsystem:windows" #
# add_flag LinkerFlags_win32 "-entry:WinMain" #
LinkerFlags_win32+=" -opt:ref" # eliminate functions that are never referenced

LinkerFlags_osx=""

LinkerFlags_wasm="--no-entry"
LinkerFlags_wasm+=" --export-dynamic" #
LinkerFlags_wasm+=" --unresolved-symbols=import-functions" #

LinkerFlags_linux=""
LinkerFlags_raspi="-fuse-ld=lld"

LinkerFlags_DEBUG="-debug"
LinkerFlags_PROD=""

# Linker Libs

LinkerLibs_win32="user32.lib kernel32.lib gdi32.lib opengl32.lib" 
# winmm.lib gdi32.lib dsound.lib Ws2_32.lib Comdlg32.lib Winspool.lib"

LinkerLibs_osx="-framework OpenGL -framework Cocoa -framework IOKit ${PROJECT_PATH}/src_v2/libs/glfw_osx/lib-universal/libglfw3.a"
LinkerLibs_wasm=""
LinkerLibs_linux=""
LinkerLibs_raspi=""

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

  if [ "${ARCH}" == "arm64" ]
  then
    CompilerFlags="${CompilerFlags} -arch arm64"
  elif [ "${ARCH}" == "intel" ]
  then
    CompilerFlags="${CompilerFlags} -arch x86_64"
  else
    echo "ERROR: Unrecognized Arch: ${ARCH}"
    exit 0
  fi

elif [ "${PLATFORM}" == "wasm" ]
then
  Compiler=$Compiler_wasm
  PlatformEntry=$PlatformEntry_wasm
  CompilerFlags=$CompilerFlags_wasm
  CompilerOutput=$CompilerOutput_wasm
  LinkerOutput=$LinkerOutput_wasm
  LinkerFlags=$LinkerFlags_wasm
  LinkerLibs=$LinkerLibs_wasm

elif [ "${PLATFORM}" == "linux" ]
then
  Compiler=$Compiler_linux
  PlatformEntry=$PlatformEntry_linux
  CompilerFlags=$CompilerFlags_linux
  CompilerOutput=$CompilerOutput_linux
  LinkerOutput=$LinkerOutput_linux
  LinkerFlags=$LinkerFlags_linux
  LinkerLibs=$LinkerLibs_linux

elif [ "${PLATFORM}" == "raspi" ]
then
  Compiler=$Compiler_raspi
  PlatformEntry=$PlatformEntry_raspi
  CompilerFlags=$CompilerFlags_raspi
  CompilerOutput=$CompilerOutput_raspi
  LinkerOutput=$LinkerOutput_raspi
  LinkerFlags=$LinkerFlags_raspi
  LinkerLibs=$LinkerLibs_raspi
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

# Exception for wasm, which doesn't care about cpu architecture
if [ $PLATFORM == "wasm" ]
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

echo "Cleaning: ${CompilerOutput} and ${LinkerOutput}"
rm -rf ${CompilerOutput} ${LinkerOutput}

echo "COMPILING..."
if [ $PLATFORM == "win32" ]
then
  $Compiler \
    $CompilerFlags \
    $EntryPath \
    -link \
      $LinkerFlags \
      $LinkerLibs \
      -OUT:${LinkerOutput}

elif [ $PLATFORM == "wasm" ]
then
  $Compiler \
    $CompilerFlags \
    -o $LinkerOutput \
    $EntryPath
  cp \
    "${PROJECT_PATH}/src_v2/platform/wasm/lumenarium_wasm_imports.js" \
    ./lumenarium_wasm_imports.js

else
  
  # Preprocessing Steps
  ConvertCsvEntry="${PROJECT_PATH}/src_v2/tools/convert_csv.c"
  $Compiler \
    -o convert_csv \
    $CompilerFlags \
    $ConvertCsvEntry \
    $LinkerLibs
  
  ./convert_csv

  echo "$Compiler -o $LinkerOutput $CompilerFlags $EntryPath $LinkerLibs"
  $Compiler -o $LinkerOutput $CompilerFlags $EntryPath $LinkerLibs

fi

echo "Finished..."
popdir
