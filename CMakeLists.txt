cmake_minimum_required(VERSION 4.3)
project(Joan C)
file(GLOB SOURCES "src/*.c")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# file(GLOB SOURCES "src/*.c")


add_compile_options(-Wall -Wno-unused-function -Wno-unused-variable)
# find_package(OpenGL REQUIRED)

add_executable(joan ${SOURCES})
include_directories(include)
add_library(joan_shared SHARED ${SOURCES})
# add_executable(joan src/main.c)
set_target_properties(joan_shared PROPERTIES OUTPUT_NAME "joan")

target_compile_options(joan PRIVATE -Wall -Wno-unused-function -Wno-unused-variable) # exculde -Wextra for now.
# set_target_properties(joan PROPERTIES LINKER_LANGUAGE CXX)
# target_link_libraries(joan PRIVATE opengl32 gdi32)
# target_link_options(
    # joan PRIVATE opengl32 gdi32)
