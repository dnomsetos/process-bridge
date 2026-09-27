include(FetchContent)

set(_pb_saved_build_shared_libs ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS ON)

FetchContent_Declare(
  unicorn
  GIT_REPOSITORY https://github.com/unicorn-engine/unicorn.git
  GIT_TAG 2.1.4)
FetchContent_MakeAvailable(unicorn)

set(BUILD_SHARED_LIBS ${_pb_saved_build_shared_libs})
unset(_pb_saved_build_shared_libs)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

add_library(pb-loader SHARED ${SRC_FILES})
target_include_directories(pb-loader PUBLIC include)
target_link_libraries(pb-loader PUBLIC unicorn pb-log pb-common)

if(SKBUILD)
  set_target_properties(pb-loader PROPERTIES BUILD_WITH_INSTALL_RPATH ON
                                             INSTALL_RPATH "$ORIGIN")

  install(TARGETS pb-loader LIBRARY DESTINATION process_bridge/_native)
  install(FILES $<TARGET_FILE:unicorn> DESTINATION process_bridge/_native)
endif()

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
