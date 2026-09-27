file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

add_library(pb-snapshotter-lib STATIC ${SRC_FILES})
target_include_directories(pb-snapshotter-lib
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(pb-snapshotter-lib PRIVATE _GNU_SOURCE)
target_link_libraries(pb-snapshotter-lib PUBLIC pb-log pb-common)

add_executable(pb-snapshotter src/main.c)
target_include_directories(pb-snapshotter PRIVATE include)
target_compile_definitions(pb-snapshotter PRIVATE _GNU_SOURCE)
target_link_libraries(pb-snapshotter PRIVATE pb-snapshotter-lib)

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
