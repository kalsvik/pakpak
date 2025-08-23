cmake_minimum_required(VERSION 3.11)
project(pakpak)

set(CMAKE_C_STANDARD 11)
set(ZLIB_BUILD_TESTING OFF)
set(CMAKE_C_FLAGS "-O3")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(src/zlib)

add_executable(pakpak src/main.c
        src/pak.c
        src/pak.h
        src/readops.c
        src/readops.h
        src/writeops.c
        src/writeops.h)

target_link_libraries(pakpak PRIVATE zlibstatic)
