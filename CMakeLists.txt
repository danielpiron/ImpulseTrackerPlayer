cmake_minimum_required(VERSION 3.8)
project(SonicMan)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(sonic main.cc mixer.cc)
target_compile_features(sonic PRIVATE cxx_std_11)
target_compile_options(sonic
                       PUBLIC
                       -Wall
                       -Wextra
                       -Wshadow
                       -Wnon-virtual-dtor
                       -Wold-style-cast
                       -Wcast-align
                       -Wunused
                       -Woverloaded-virtual
                       -Wpedantic
                       -Wsign-conversion
                       -Wduplicated-cond
                       -Wduplicated-branches
                       -Wlogical-op
                       -Wnull-dereference
                       -Wuseless-cast
                       -Wdouble-promotion
                       -Wformat=2
                       )
target_link_libraries(sonic portaudio)
