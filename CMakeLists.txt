cmake_minimum_required(VERSION 3.20)

project(
  process-bridge
  VERSION 0.1.0
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(pb-common INTERFACE)

target_include_directories(pb-common
                           INTERFACE ${PROJECT_SOURCE_DIR}/common/include)

option(PROCESS_BRIDGE_BUILD_SNAPSHOTTER "Build snapshotter" OFF)
option(PROCESS_BRIDGE_BUILD_LOADER "Build loader" OFF)

add_subdirectory(utilities/log)

include(CTest)

if(PROCESS_BRIDGE_BUILD_SNAPSHOTTER)
  add_subdirectory(snapshotter)
endif()

if(PROCESS_BRIDGE_BUILD_LOADER)
  add_subdirectory(loader)
endif()
