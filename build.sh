#!/bin/bash

CC=gcc
CFLAGS="-std=c89 -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter -Wno-declaration-after-statement -Wno-unused-but-set-variable -pedantic -g -O3"
LDFLAGS="-I/usr/include/postgresql -lpq"

SRC_DIR="src"
BUILD_DIR="build"
BIN_DIR="$BUILD_DIR/bin"
OBJ_DIR="$BUILD_DIR/obj"
ASSEMBLY_DIR="$BUILD_DIR/asm"

EXECUTABLE="$BIN_DIR/andlifecare"
SOURCES=$(find "$SRC_DIR" -type f -name "*.c")
OBJECTS=$(echo "$SOURCES" | sed "s|$SRC_DIR/|$OBJ_DIR/|g; s|\.c$|\.o|g")
ASSEMBLY_FILES=$(echo "$SOURCES" | sed "s|$SRC_DIR/|$ASSEMBLY_DIR/|g; s|\.c$|\.s|g")

compile_objects() {
    for source_file in $SOURCES; do
        object_file=$(echo "$source_file" | sed "s|$SRC_DIR/|$OBJ_DIR/|g; s|\.c$|\.o|")
        mkdir -p "$(dirname "$object_file")"
        $CC $CFLAGS -I"$SRC_DIR" -c "$source_file" -o "$object_file" $LDFLAGS
    done
}

generate_assembly() {
    for source_file in $SOURCES; do
        assembly_file=$(echo "$source_file" | sed "s|$SRC_DIR/|$ASSEMBLY_DIR/|g; s|\.c$|\.s|")
        mkdir -p "$(dirname "$assembly_file")"
        $CC -S -I"$SRC_DIR" "$source_file" -o "$assembly_file" $LDFLAGS
    done
}

build() {
    mkdir -p "$BIN_DIR" "$OBJ_DIR" "$ASSEMBLY_DIR"
    compile_objects
    $CC $CFLAGS $OBJECTS -o "$EXECUTABLE" $LDFLAGS
    generate_assembly
}

clean() {
    rm -rf "$BUILD_DIR"
}

# Clean build directories before building
clean
build

exit 0
