cmake_minimum_required(VERSION 3.8)
project(SonicMan)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(sonic main.cc)
target_compile_features(sonic PRIVATE cxx_std_11)
target_compile_options(sonic
                       INTERFACE
                       -Wall
                       -Wextra
                       -Wshadow
                       -Wnon-virtual-dtor
                       -Wpedantic
                       )
target_link_libraries(sonic portaudio)
