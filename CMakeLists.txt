cmake_minimum_required(VERSION 3.20)

project(process-bridge LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/native/src/*.c")

list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/native/src/main.c")

add_library(process-bridge-lib SHARED ${SRC_FILES})

target_include_directories(process-bridge-lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/native/include)

target_compile_definitions(process-bridge-lib PRIVATE _GNU_SOURCE)

add_executable(process-bridge native/src/main.c)

set_target_properties(process-bridge-lib PROPERTIES OUTPUT_NAME
                                                    process-bridge-lib)

set_target_properties(process-bridge PROPERTIES BUILD_RPATH "\$ORIGIN"
                                                INSTALL_RPATH "\$ORIGIN")

target_compile_definitions(process-bridge PRIVATE _GNU_SOURCE)

target_link_libraries(process-bridge PRIVATE process-bridge-lib)

install(TARGETS process-bridge-lib LIBRARY DESTINATION .)
install(TARGETS process-bridge RUNTIME DESTINATION .)

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests/native)
endif()
