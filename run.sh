#!/bin/bash

BUILD_DIR="cmake-files"
EXECUTABLE_NAME="threads.out"

set -e

echo "--- Starting build process ---"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory: $BUILD_DIR"
    mkdir "$BUILD_DIR"
fi

echo "Entering build directory..."
cd "$BUILD_DIR"

echo "Running CMake..."
cmake ..

echo "Building project..."
make

echo "Returning to project root..."
cd ..

echo "--- Build complete. Running executable: $EXECUTABLE_NAME ---"
echo ""
./"$EXECUTABLE_NAME"
