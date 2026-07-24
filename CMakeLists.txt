cmake_minimum_required(VERSION 4.2)

project(process-bridge LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

add_executable(process-bridge src/main.c src/platform/linux/breakpoint.c src/platform/linux/process_info.c src/platform/linux/maps_parser.c)

target_compile_definitions(process-bridge PRIVATE _GNU_SOURCE)

target_include_directories(process-bridge
    PRIVATE
        include
)
