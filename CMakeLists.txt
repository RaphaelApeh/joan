cmake_minimum_required(VERSION 4.3) # latest
project(Joan C CXX)
file(GLOB SOURCES "src/*.c" "src/vendor/*.c" "src/vendor/linenoise/*.cpp")
# file(GLOB SOURCES "src/*.c")

find_package(OpenGL REQUIRED)
add_executable(joan ${SOURCES})
include_directories(include)
set_target_properties(joan PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(joan PRIVATE opengl32 gdi32)
# target_link_options(
    # joan PRIVATE opengl32 gdi32)
