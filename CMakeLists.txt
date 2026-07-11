cmake_minimum_required(VERSION 4.3)
project(Joan C)
file(GLOB SOURCES "src/*.c")
# file(GLOB SOURCES "src/*.c")

# find_package(OpenGL REQUIRED)

add_executable(joan ${SOURCES})
include_directories(include)
add_library(joan_shared SHARED ${SOURCES})
# add_executable(joan src/main.c)
set_target_properties(joan_shared PROPERTIES OUTPUT_NAME "joan")

# set_target_properties(joan PROPERTIES LINKER_LANGUAGE CXX)
# target_link_libraries(joan PRIVATE opengl32 gdi32)
# target_link_options(
    # joan PRIVATE opengl32 gdi32)
