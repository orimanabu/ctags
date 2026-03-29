#!/bin/sh
# Build ctags as a WebAssembly module.
#
# Produces ctags-wasm.js and ctags-wasm.wasm in the build directory.
# The module exports:
#   ctags_init()                          - initialize (JSON output + sort)
#   ctags_parse_buffer(name, data, size)  - parse source from a memory buffer
#   ctags_set_output_format(fmt)          - change output format at runtime
#
# Output mimics: ctags --oneshot=<filename> --sort -o - --output-format json
#
# Requirements:
#   - Emscripten SDK active in PATH (emcc, emconfigure, emmake)
#   - curl or wget (to download Jansson if not already built)
#   - autoconf / automake (only needed when building from a git checkout)
#
# Usage:
#   ./misc/build-wasm.sh [--build-dir DIR] [--jansson-dir DIR]
#
#     --build-dir DIR     directory for the out-of-tree build  [./build-wasm]
#     --jansson-dir DIR   pre-built Jansson WASM tree          [auto-fetched]
#
set -e

CTAGS_SRC="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$CTAGS_SRC/build-wasm}"
JANSSON_VERSION="2.14"
JANSSON_DIR="${JANSSON_DIR:-$BUILD_DIR/jansson-$JANSSON_VERSION}"

# Parse CLI flags
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir)  BUILD_DIR="$2";  shift 2 ;;
        --jansson-dir) JANSSON_DIR="$2"; shift 2 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

# Verify Emscripten is available
if ! command -v emcc >/dev/null 2>&1; then
    echo "Error: emcc not found. Activate the Emscripten SDK first:" >&2
    echo "  source /path/to/emsdk/emsdk_env.sh" >&2
    exit 1
fi

echo "==> Emscripten: $(emcc --version | head -1)"
echo "==> ctags source : $CTAGS_SRC"
echo "==> build dir    : $BUILD_DIR"
echo "==> Jansson dir  : $JANSSON_DIR"

mkdir -p "$BUILD_DIR"

# Build Jansson for WASM if not already built
JANSSON_LIB="$JANSSON_DIR/src/.libs/libjansson.a"
if [ ! -f "$JANSSON_LIB" ]; then
    JANSSON_TAR="$BUILD_DIR/jansson-$JANSSON_VERSION.tar.gz"
    if [ ! -f "$JANSSON_TAR" ]; then
        echo "==> Downloading Jansson $JANSSON_VERSION ..."
        JANSSON_URL="https://github.com/akheron/jansson/releases/download/v$JANSSON_VERSION/jansson-$JANSSON_VERSION.tar.gz"
        if command -v curl >/dev/null 2>&1; then
            curl -L -o "$JANSSON_TAR" "$JANSSON_URL"
        else
            wget -O "$JANSSON_TAR" "$JANSSON_URL"
        fi
    fi
    echo "==> Extracting Jansson ..."
    tar -C "$BUILD_DIR" -xzf "$JANSSON_TAR"
    echo "==> Building Jansson for WASM ..."
    (
        cd "$JANSSON_DIR"
        emconfigure ./configure --disable-shared --disable-dependency-tracking
        emmake make
    )
fi

JANSSON_CFLAGS="-I$JANSSON_DIR/src"
JANSSON_LIBS="$JANSSON_LIB"

# Generate configure script if building from a git checkout
if [ ! -f "$CTAGS_SRC/configure" ]; then
    echo "==> Running autogen.sh ..."
    (cd "$CTAGS_SRC" && ./autogen.sh)
fi

# autoconf refuses an out-of-tree configure when the source directory has
# already been configured in-tree (i.e. config.status exists there).
# Run "make distclean" in the source tree to clear it first.
if [ -f "$CTAGS_SRC/config.status" ]; then
    echo "==> Source directory has an in-tree build (config.status found)."
    echo "    Running 'make distclean' in $CTAGS_SRC to allow out-of-tree WASM build ..."
    make -C "$CTAGS_SRC" distclean
fi

# Configure ctags for WASM (out-of-tree, from BUILD_DIR)
echo "==> Configuring ctags for WASM ..."
(
    cd "$BUILD_DIR"
    JANSSON_CFLAGS="$JANSSON_CFLAGS" \
    JANSSON_LIBS="$JANSSON_LIBS" \
    emconfigure "$CTAGS_SRC/configure" \
        --disable-seccomp \
        --disable-xml \
        --disable-yaml \
        --disable-pcre2 \
        --disable-iconv
)

# Build the native packcc (must use the host compiler, not emcc)
echo "==> Building native packcc ..."
cc -fsigned-char -DPCC_USE_SYSTEM_STRNLEN \
    -o "$BUILD_DIR/packcc" \
    "$CTAGS_SRC/misc/packcc/src/packcc.c"

# Build the WASM module
echo "==> Building ctags WASM module ..."
emmake make -C "$BUILD_DIR" wasm

echo ""
echo "Done. Output files:"
echo "  $BUILD_DIR/ctags-wasm.js"
echo "  $BUILD_DIR/ctags-wasm.wasm"
