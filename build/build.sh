SCRIPT_REL_DIR=$(dirname "${BASH_SOURCE[0]}")
$SCRIPT_REL_DIR/build_.sh prod osx arm64
# $SCRIPT_REL_DIR/build_.sh debug wasm intel