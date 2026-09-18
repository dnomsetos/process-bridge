cmake_minimum_required(VERSION 3.20)

project(process-bridge-package LANGUAGES NONE)

include(ExternalProject)

set(PACKAGE_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/package/process_bridge/bin")

ExternalProject_Add(
  process-bridge-x86_64
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
  CMAKE_ARGS -DCMAKE_C_COMPILER=clang
             -DCMAKE_C_COMPILER_TARGET=x86_64-pc-linux-gnu
             -DCMAKE_BUILD_TYPE=Release
             -DBUILD_TESTING=OFF
             -DCMAKE_INSTALL_PREFIX=${PACKAGE_BIN_DIR}/x86_64
  BUILD_ALWAYS TRUE
  INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>)

ExternalProject_Add(
  process-bridge-i386
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
  CMAKE_ARGS -DCMAKE_C_COMPILER=clang
             -DCMAKE_C_COMPILER_TARGET=i386-pc-linux-gnu
             -DCMAKE_BUILD_TYPE=Release
             -DBUILD_TESTING=OFF
             -DCMAKE_INSTALL_PREFIX=${PACKAGE_BIN_DIR}/i386
  BUILD_ALWAYS TRUE
  INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>)

install(
  DIRECTORY "${PACKAGE_BIN_DIR}/"
  DESTINATION process_bridge/bin
  FILE_PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)
