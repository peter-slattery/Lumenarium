#!/bin/bash

# --------------------------------------------
#            Usage 

VALID_VALUES_COMPILER=("clang" "clang++" "msvc")
VALID_VALUES_MODE=("debug" "release")
VALID_VALUES_PLATFORM=("win32" "osx" "linux" "raspi" "wasm")
VALID_VALUES_ARCH=("x64" "arm64")
VALID_VALUES_PACKAGE=("true" "false")

printf_r () {
  printf "\e[31m$@\e[0m\n"
}

printf_g () {
  printf "\e[32m%s\e[0m\n" "$@"
}

print_usage () {
  printf "\n"
  printf "Build Command Syntax:\n"
  printf_g "  $0 [compiler] [mode] [platform] [arch] [package]"
  printf "\n"
  printf "Compiler Options:\n"
  printf "  \e[32m%s\e[m\n" "${VALID_VALUES_COMPILER[@]}"
  printf "\n"
  printf "Release Mode Options:\n"
  printf "  \e[32m%s\e[m\n" "${VALID_VALUES_MODE[@]}"
  printf "\n"
  printf "Platform Options:\n"
  printf "  \e[32m%s\e[m\n" "${VALID_VALUES_PLATFORM[@]}"
  printf "\n"
  printf "Arch Options: \n"
  printf "  \e[32m%s\e[m\n" "${VALID_VALUES_ARCH[@]}"
  printf "\n"
  printf "Package Options:\n"
  printf_g "  'package' or no flag to omit packaging\n"
  printf "\n"
  printf "Examples:\n"
  printf "  $0 clang debug osx arm64\n"
  printf "  $0 msvc release win32 x64 package\n"
}

OPTS=()
for ((i=1; i<=$#; i+=1)); do
  OPTS+=(${!i})
done

if [ "${OPTS[0]}" == "-h" ] || [ "${OPTS[0]}" == "--help" ]
then
  print_usage
  exit 1
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
# Project Directory Identification

OLD_PATH=$(pwd)

BUILD_SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")
pushdir $BUILD_SCRIPT_DIR
pushdir ..
PROJECT_PATH=$(pwd)
popdir
popdir

BLD_DIR="${PROJECT_PATH}/build"
SRC_DIR="${PROJECT_PATH}/src"
OUT_DIR="${PROJECT_PATH}/run_tree"

# --------------------------------------------
# Input Flag Settings

INPUT_FLAG_UNSET="unset"

COMPILER=$INPUT_FLAG_UNSET
MODE=$INPUT_FLAG_UNSET
PLATFORM=$INPUT_FLAG_UNSET
ARCH=$INPUT_FLAG_UNSET
PACKAGE=$INPUT_FLAG_UNSET

# --------------------------------------------
# Create a local build file if there isn't one
# using local context to determine defaults

BLD_LOCAL_FILE="${BLD_DIR}/build_local.sh"

if [ ! -f $BLD_LOCAL_FILE ] 
then

  printf "Creating a build/build_local.sh file for you."
  printf "  Path: ${BLD_LOCAL_FILE}"
  printf "This file is excluded in the .gitignore. It is for you to set local compilation targets"

  touch $BLD_LOCAL_FILE
  printf "#!/bin/bash"        >> $BLD_LOCAL_FILE
  printf                      >> $BLD_LOCAL_FILE
  printf "COMPILER=\"clang\"" >> $BLD_LOCAL_FILE
  printf "MODE=\"debug\""     >> $BLD_LOCAL_FILE
  printf "PLATFORM=\"osx\""   >> $BLD_LOCAL_FILE
  printf "ARCH=\"arm64\""     >> $BLD_LOCAL_FILE
  printf "PACKAGE=\"false\""  >> $BLD_LOCAL_FILE
  printf "TEST_FILE=\"\""     >> $BLD_LOCAL_FILE
fi

# --------------------------------------------
# Call Local Build File

source ${BLD_LOCAL_FILE}

# --------------------------------------------
# Use command line arguments to override local
# build file

if [ "${#OPTS[@]}" -gt "0" ]; then
  OPTS_COUNT="${#OPTS[@]}"
  if [ $OPTS_COUNT -lt "4" ] || [ $OPTS_COUNT -gt "5" ]; then
    printf_r "Error: Incorrect number of arguments supplied"
    printf   "       You must either supply all or none of the build script arguments\n"
    print_usage
    exit 1
  fi
  
  COMPILER=${OPTS[0]}
  MODE=${OPTS[1]}
  PLATFORM=${OPTS[2]}
  ARCH=${OPTS[3]}

  PACkaGE="false"
  if [ $OPTS_COUNT -eq "5" ]; then
    if [ "${OPTS[4]}" == "package" ]; then
      PACKAGE="true"
    else
      printf_r "Error: Invalid package command provided: ${PACKAGE}"
      printf   "  You must either supply the 'package' flag or omit it"
      exit 1
    fi
  fi
fi

# --------------------------------------------
# Verify valid values for all inputs

ALL_VALID_VALUES="true"

check_valid_flag () {
  local VALID_VALUES_NAME=$1[@]
  local VALID_VALUES=("${!VALID_VALUES_NAME}")
  local VALUE_GIVEN=$2
  local VALUE_ID=$3
  
  if [[ ! " ${VALID_VALUES[*]} " =~ " ${VALUE_GIVEN} " ]]; then
    printf_r "Error: Invalid ${VALUE_ID} provided: ${VALUE_GIVEN}"
    printf   "  Must be one of: "
    printf_g "${VALID_VALUES[*]}\n"
    ALL_VALID_VALUES="false"
  fi
}

check_valid_flag VALID_VALUES_COMPILER $COMPILER "compiler"
check_valid_flag VALID_VALUES_MODE     $MODE     "mode"
check_valid_flag VALID_VALUES_PLATFORM $PLATFORM "platform"
check_valid_flag VALID_VALUES_ARCH     $ARCH     "arch"

if [[ ! " ${VALID_VALUES_PACKAGE[*]} " =~ " ${PACKAGE} " ]]; then
  printf_r "Error: Invalid package provided: ${PACKAGE}"
  printf   "  You must either supply the 'package' flag or omit it"
  ALL_VALID_VALUES="false"
fi

if [ "${ALL_VALID_VALUES}" != "true" ]; then 
  exit 1
fi

if [ "${COMPILER}" == "clang" ] || [ "${COMPILER}" == "clang++" ]; then
  LINKER=${COMPILER}
elif [ "${COMPILER}" == "msvc" ]; then
  LINKER="link"
fi

printf "Compiler: "
  printf_g "${COMPILER}"
printf "Mode:     "
  printf_g "${MODE}"
printf "Platform: "
  printf_g "${PLATFORM}"
printf "Arch:     "
  printf_g "${ARCH}"
printf "Package:  "
  printf_g "${PACKAGE}"

# --------------------------------------------
# Hooks Identification

HOOK_PREBUILD="${BLD_DIR}/hook_prebuild.sh"
HOOK_POSTBUILD="${BLD_DIR}/hook_postbuild.sh"
HOOK_PRELINK="${BLD_DIR}/hook_prelink.sh"
HOOK_POSTLINK="${BLD_DIR}/hook_postlink.sh"

printf "\nBuild Hooks:\n"
if [ -f "${HOOK_PREBUILD}" ]; then
  printf "  Pre  Build: ${HOOK_PREBUILD##*/}\n"
else
  HOOK_PREBUILD=""
fi

if [ -f "${HOOK_POSTBUILD}" ]; then
  printf "  Post Build: ${HOOK_POSTBUILD##*/}\n"
else
  HOOK_POSTBUILD=""
fi

if [ -f "${HOOK_PRELINK}" ]; then
  printf "  Pre  Link: ${HOOK_PRELINK##*/}\n"
else
  HOOK_PRELINK=""
fi

if [ -f "${HOOK_POSTLINK}" ]; then
  printf "  Post Link: ${HOOK_POSTLINK##*/}\n"
else
  HOOK_POSTLINK=""
fi

# --------------------------------------------
# File Parsing Helpers

trim() {
  local var="$*"

  # remove leading whitespace characters
  var="${var#"${var%%[![:space:]]*}"}"

  # remove trailing whitespace characters
  var="${var%"${var##*[![:space:]]}"}"

  echo "${var}"
}

load_file_into_lines_array() {
  FILE=$1
  local FILE_LINES_RAW=()
  IFS=$'\r\n'
  GLOBIGNORE='*'
  command eval 'FILE_LINES_RAW=($(cat $FILE))'

  FILE_LINES=()
  for i in "${FILE_LINES_RAW[@]}"; do
    if [ "${i:0:1}" != "#" ]; then
      # strip any trailing comments off the end
      # this lets you do things like:
      #   "compiler>msvc>-FC # full path error"
      # where you want to explain a flag
      local LINE=$i
      local LINE_NO_TRAILING_COMMENT="${LINE% #*}"
      FILE_LINES+=($LINE_NO_TRAILING_COMMENT)
    fi
  done
}

parse_flags_from_selectors() {
  SELECTORS=()
  for ((i=1; i<=$#; i+=1)); do
    SELECTORS+=(${!i})
  done
  
  FLAGS=()
  for i in "${FILE_LINES[@]}"; do
    LINE=$i
    FLAG="${LINE##*>}"
  
    INCLUDE_FLAG="true"
    while [ "${LINE}" != "${FLAG}" ]; do
      NEXT_SELECTOR="${LINE%%>*}"
      LINE="${LINE#*>}"
      if [[ ! " ${SELECTORS[@]} " =~ " ${NEXT_SELECTOR} " ]]; then
        INCLUDE_FLAG="false"
        break
      fi
      
    done

    if [ "${INCLUDE_FLAG}" == "true" ]; then
      FLAG=$(trim "${FLAG}")
      FLAG=$(eval "echo $FLAG")
      FLAGS+=($FLAG)
    fi
  done
}

load_file_into_lines_array "${BLD_DIR}/build_flags.sh"

# --------------------------------------------
# Assemble Flags

parse_flags_from_selectors "compiler" $COMPILER $MODE $PLATFORM $ARCH
COMPILER_FLAGS=(${FLAGS[@]})

parse_flags_from_selectors "compiler" "input" $PLATFORM $ARCH
COMPILER_INPUTS=(${FLAGS[@]})

parse_flags_from_selectors "linker" "flags" $MODE $PLATFORM $ARCH
LINKER_FLAGS=(${FLAGS[@]})

parse_flags_from_selectors "linker" "libs" $LINKER $MODE $PLATFORM $ARCH
LINKER_LIBRARIES=(${FLAGS[@]})

parse_flags_from_selectors "linker" "output" $LINKER $MODE $PLATFORM $ARCH
LINKER_OUTPUT=(${FLAGS[@]})

# --------------------------------------------
# Create the Run Tree path

OUT_PATH="${OUT_DIR}/${PLATFORM}/${ARCH}/${MODE}"

if [ ! -d $OUT_PATH ]; then
  mkdir -p $OUT_PATH
fi

# --------------------------------------------
# Compile The Program

printf "\nBeginning Compilation...\n"
pushd $OUT_PATH

find . -name "*" -delete

if [[ -f ${HOOK_PREBUILD} ]]; then
  source "${HOOK_PREBUILD}"
fi

COMPILATION_SUCCESS="true"
COMPILER_OUTPUT=()
FAILED_COMPILES=()
for i in "${COMPILER_INPUTS[@]}"; do
  INPUT="${i}"
  
  INPUT_FILE="${INPUT##*/}"
  INPUT_EXTENSION="${INPUT_FILE##*.}"
  INPUT_NAME="${INPUT_FILE%.*}"
  OUTPUT_FILE="${INPUT_NAME}_${INPUT_EXTENSION}.o"
  
  COMPILER_ARGS="-o ${OUTPUT_FILE} -c ${COMPILER_FLAGS[@]} -DPLATFORM_$PLATFORM=1 -DMODE_$MODE=1 -DARCH_$ARCH=1 $INPUT"

  # echo $COMPILER $COMPILER_ARGS
  eval $COMPILER $COMPILER_ARGS
  if [ $? -eq 0 ]; then
    COMPILER_OUTPUT+=(${OUTPUT_FILE})
  else
    COMPILATION_SUCCESS="false"
    FAILED_COMPILES+=(${OUTPUT_FILE})
  fi

  # TODO: if the output file was a .dll or .lib we don't want to include
  # those in the final compilation gather
done

printf "\nCompiler Output\n"
if [ ${#COMPILER_OUTPUT[@]} -gt 0 ]; then
  printf "  %s \e[32m[SUCCESS]\e[0m\n" "${COMPILER_OUTPUT[@]}"
fi
if [ ${#FAILED_COMPILES[@]} -gt 0 ]; then
  printf "  %s \e[31m[FAILED]\e[0m\n" "${FAILED_COMPILES[@]}"
fi
printf "\n"

if [[ -f ${HOOK_POSTBUILD} ]]; then
  source "${HOOK_POSTBUILD}"
fi

if [ $COMPILATION_SUCCESS != "true" ]; then
  printf "Compilation Failed.\n  Exiting..."
  exit 1
fi

if [[ -f ${HOOK_PRELINK} ]]; then
  source "${HOOK_PRELINK}"
fi

LINKER_ARGS="-o ${LINKER_OUTPUT} ${COMPILER_OUTPUT[@]} ${LINKER_FLAGS[@]} ${LINKER_LIBRARIES[@]}"

printf "Linking...\n"
echo $LINKER $LINKER_ARGS
eval $LINKER $LINKER_ARGS
if [ $? -eq 0 ]; then
  printf   "  Link: "
  printf_g "[SUCCEEDED]"
else
  printf   "\n  Link: "
  printf_r "[FAILED]"
fi

if [[ -f ${HOOK_POSTLINK} ]]; then
  source "${HOOK_POSTLINK}"
fi

popdir

exit 0