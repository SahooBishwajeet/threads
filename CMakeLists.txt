cmake_minimum_required(VERSION 3.15)
project(Threads)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

find_package(OpenCV REQUIRED)

add_executable(threads.out main.cpp)

target_link_libraries(threads.out ${OpenCV_LIBS})
