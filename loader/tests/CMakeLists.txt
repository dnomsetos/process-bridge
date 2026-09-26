set(PB_LOADER_TEST_SOURCES
    test_cpu_state_dump_i386.c test_cpu_state_dump_x86_64.c
    test_cpu_state_dump_dispatch.c test_dump_snapshot.c)

foreach(TEST_SRC ${PB_LOADER_TEST_SOURCES})
  get_filename_component(TEST_BASE ${TEST_SRC} NAME_WE)
  add_executable(${TEST_BASE} "${TEST_SRC}")

  target_include_directories(${TEST_BASE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${TEST_BASE} PRIVATE pb-loader-lib pb-test)

  add_test(NAME ${TEST_BASE} COMMAND ${TEST_BASE})
  set_tests_properties(${TEST_BASE} PROPERTIES TIMEOUT 60)
endforeach()
