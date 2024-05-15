#!/bin/sh

BUILD_DIR="./build"

CC="clang"

CORE_CFLAGS="-Wall -Wextra -pedantic"
DEBUG_CFLAGS="-ggdb -fsanitize=address"

LIBS=(
    "./utils.c"
    "./hvm.c"
    "./hotaru.c"
    "./hparser.c"
)

if [ ! -d $BUILD_DIR ]; then
    mkdir $BUILD_DIR
fi

echo "Building $BUILD_DIR/hotaru"
$CC $CORE_CFLAGS $DEBUG_CFLAGS -o $BUILD_DIR/hotaru "${LIBS[@]}" ./main.c

echo "Building $BUILD_DIR/test"
$CC $CORE_CFLAGS $DEBUG_CFLAGS -o $BUILD_DIR/test "${LIBS[@]}" ./test.c


echo "Building $BUILD_DIR/hvm"
$CC $CORE_CFLAGS -Os -o $BUILD_DIR/hvm ./hvmmain.c ./hvm.c ./utils.c
