cmake_minimum_required(VERSION 4.3) # latest
project(Joan)
file(GLOB SOURCES "src/*.c" "src/builtins/*.c")
add_executable(joan ${SOURCES})
include_directories(include)