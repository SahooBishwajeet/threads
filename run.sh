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
if [ $? -ne 0 ]; then
    echo "Build failed. Exiting."
    exit 1
fi
echo "Returning to project root..."
cd ..

echo "--- Build complete. You can now run the executable: $EXECUTABLE_NAME"
