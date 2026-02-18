cmake_minimum_required(VERSION 3.10)
project(craftos-base C)

# Set C standard to ANSI C (C89/C90)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Collect source files
file(GLOB SOURCES "src/*.c" "src/apis/*.c")
set(LUA_SOURCES
    craftos2-lua/src/error_info.c
    craftos2-lua/src/lapi.c
    craftos2-lua/src/lcode.c
    craftos2-lua/src/lctype.c
    craftos2-lua/src/ldebug.c
    craftos2-lua/src/ldo.c
    craftos2-lua/src/ldump.c
    craftos2-lua/src/lfunc.c
    craftos2-lua/src/lgc.c
    craftos2-lua/src/linit.c
    craftos2-lua/src/llex.c
    craftos2-lua/src/lmem.c
    craftos2-lua/src/lobject.c
    craftos2-lua/src/lopcodes.c
    craftos2-lua/src/lparser.c
    craftos2-lua/src/lstate.c
    craftos2-lua/src/lstring.c
    craftos2-lua/src/ltable.c
    craftos2-lua/src/ltm.c
    craftos2-lua/src/lundump.c
    craftos2-lua/src/lvm.c
    craftos2-lua/src/lzio.c
    craftos2-lua/src/lauxlib.c
    craftos2-lua/src/lbaselib.c
    craftos2-lua/src/lbitlib.c
    craftos2-lua/src/lcorolib.c
    craftos2-lua/src/ldblib.c
    craftos2-lua/src/liolib.c
    craftos2-lua/src/lmathlib.c
    craftos2-lua/src/loslib.c
    craftos2-lua/src/lstrlib.c
    craftos2-lua/src/ltablib.c
    craftos2-lua/src/lutf8lib.c
    craftos2-lua/src/loadlib.c
)
list(APPEND SOURCES ${LUA_SOURCES})

# Check for optional headers
include(CheckIncludeFile)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/statvfs.h HAVE_SYS_STATVFS_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(dirent.h HAVE_DIRENT_H)

# Create config.h
configure_file(
    "src/config.h.in"
    "config.h"
)

# Create library
add_library(craftos-base ${SOURCES})

# Configure include directories
target_include_directories(craftos-base
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/craftos2-lua/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/craftos2-lua/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Optional: Install targets for embedding projects
install(TARGETS craftos-base
    EXPORT craftosbaseTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

# Export configuration for find_package()
install(EXPORT craftosbaseTargets
    FILE craftosbaseTargets.cmake
    DESTINATION lib/cmake/craftosbase
)

add_subdirectory(example)
