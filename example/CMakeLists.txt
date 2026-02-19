cmake_minimum_required(VERSION 3.10)
project(craftos-lite C)

find_package(SDL3)
find_package(CURL)

# Add the executable
add_executable(craftos-lite craftos-lite.c)

# Link against the parent library
target_link_libraries(craftos-lite PRIVATE craftos-base m SDL3 curl)

# Include directories
target_include_directories(craftos-lite
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
