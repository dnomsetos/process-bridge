cmake_minimum_required(VERSION 3.20)

project(process-bridge LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/native/src/*.c")

list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/native/src/main.c")

function(add_arch_targets ARCH_SUFFIX)
  set(FLAGS ${ARGN})

  set(LIB_NAME process-bridge-lib-${ARCH_SUFFIX})
  set(EXE_NAME process-bridge-${ARCH_SUFFIX})

  add_library(${LIB_NAME} STATIC ${SRC_FILES})

  target_include_directories(${LIB_NAME}
                             PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/native/include)

  target_compile_definitions(${LIB_NAME} PRIVATE _GNU_SOURCE)

  target_compile_options(${LIB_NAME} PUBLIC ${FLAGS})

  target_link_options(${LIB_NAME} PUBLIC ${FLAGS})

  add_executable(${EXE_NAME} native/src/main.c)

  target_compile_options(${EXE_NAME} PRIVATE ${FLAGS})

  target_link_options(${EXE_NAME} PRIVATE ${FLAGS})

  target_link_libraries(${EXE_NAME} PRIVATE ${LIB_NAME})

  install(TARGETS ${EXE_NAME} DESTINATION process_bridge/bin)

endfunction()

add_arch_targets(x64 -m64)

add_arch_targets(x86 -m32)
