#!/bin/sh

BUILD_DIR="./build"

CC="clang"

OTHER_CFLAGS=""
DEBUG_CFLAGS="-ggdb -fsanitize=address"
CFLAGS="-Wall -Wextra -pedantic $OTHER_CFLAGS $DEBUG_CFLAGS"

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
$CC $CFLAGS -o $BUILD_DIR/hotaru "${LIBS[@]}" ./main.c

echo "Building $BUILD_DIR/test"
$CC $CFLAGS -o $BUILD_DIR/test "${LIBS[@]}" ./test.c

