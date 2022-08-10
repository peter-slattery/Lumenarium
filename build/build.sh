set -e

SCRIPT_REL_DIR=$(dirname "${BASH_SOURCE[0]}")
$SCRIPT_REL_DIR/build_.sh prod raspi arm64
# $SCRIPT_REL_DIR/build_.sh debug wasm intel

# pushd "run_tree/raspi/arm64/debug"
# clang -o lumenarium /home/pi/dev/Lumenarium/src_v2/platform/raspi/lumenarium_first_raspi.c -lm
# popd
