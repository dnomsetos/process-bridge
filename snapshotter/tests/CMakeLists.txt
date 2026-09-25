set(PB_NATIVE_TEST_SOURCES test_maps_parser.c test_process_info.c
                           test_mappings_reader.c)

set(TARGET_FIXTURE_NAME pb-test-target)

add_executable(${TARGET_FIXTURE_NAME} fixtures/target.c)

target_compile_options(${TARGET_FIXTURE_NAME} PRIVATE -fno-pie -O0)

target_link_options(${TARGET_FIXTURE_NAME} PRIVATE -no-pie)

foreach(TEST_SRC ${PB_NATIVE_TEST_SOURCES})
  get_filename_component(TEST_BASE ${TEST_SRC} NAME_WE)

  add_executable(${TEST_BASE} "${TEST_SRC}")

  target_include_directories(${TEST_BASE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  target_link_libraries(${TEST_BASE} PRIVATE pb-snapshotter-lib)

  add_test(NAME ${TEST_BASE} COMMAND ${TEST_BASE})

  set_tests_properties(${TEST_BASE} PROPERTIES TIMEOUT 60)
endforeach()

add_executable(test_breakpoint test_breakpoint.c)

target_include_directories(test_breakpoint PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(test_breakpoint PRIVATE pb-snapshotter-lib)

add_dependencies(test_breakpoint ${TARGET_FIXTURE_NAME})

target_compile_definitions(
  test_breakpoint
  PRIVATE PB_TEST_TARGET_PATH="$<TARGET_FILE:${TARGET_FIXTURE_NAME}>")

add_test(NAME test_breakpoint COMMAND test_breakpoint)

set_tests_properties(test_breakpoint PROPERTIES TIMEOUT 60)
