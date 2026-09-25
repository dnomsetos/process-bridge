include(FetchContent)

FetchContent_Declare(
  unicorn
  GIT_REPOSITORY https://github.com/unicorn-engine/unicorn.git
  GIT_TAG 2.1.4)

FetchContent_MakeAvailable(unicorn)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

add_library(pb-loader-lib STATIC ${SRC_FILES})
target_include_directories(pb-loader-lib PUBLIC include)
target_link_libraries(pb-loader-lib PUBLIC unicorn pb-log pb-common)

add_executable(pb-loader src/main.c)
target_include_directories(pb-loader PRIVATE include)
target_link_libraries(pb-loader PRIVATE pb-loader-lib)

include(CTest)

if(BUILD_TESTING)
  # add_subdirectory(tests)
endif()
