cmake_minimum_required(VERSION 4.3) # latest
project(Joan)
file(GLOB SOURCES "src/*.c" "src/vendor/*.c")
# file(GLOB SOURCES "src/*.c")

find_package(OpenGL REQUIRED)
add_executable(joan ${SOURCES})
include_directories(include)
target_link_libraries(joan PRIVATE opengl32 gdi32)
# target_link_options(
    # joan PRIVATE opengl32 gdi32)
