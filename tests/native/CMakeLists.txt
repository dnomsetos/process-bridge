set(PB_NATIVE_TEST_SOURCES test_maps_parser.c test_process_info.c
                           test_mappings_reader.c)

function(add_native_tests ARCH_SUFFIX)
  set(FLAGS ${ARGN})

  set(LIB_NAME process-bridge-lib-${ARCH_SUFFIX})

  set(TARGET_FIXTURE_NAME pb-test-target-${ARCH_SUFFIX})
  add_executable(${TARGET_FIXTURE_NAME} fixtures/target.c)
  target_compile_options(${TARGET_FIXTURE_NAME} PRIVATE ${FLAGS} -fno-pie -O0)
  target_link_options(${TARGET_FIXTURE_NAME} PRIVATE ${FLAGS} -no-pie)

  foreach(TEST_SRC ${PB_NATIVE_TEST_SOURCES})
    get_filename_component(TEST_BASE ${TEST_SRC} NAME_WE)
    set(TEST_NAME ${TEST_BASE}_${ARCH_SUFFIX})

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_compile_options(${TEST_NAME} PRIVATE ${FLAGS})
    target_link_options(${TEST_NAME} PRIVATE ${FLAGS})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${TEST_NAME} PRIVATE ${LIB_NAME})

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 60)
  endforeach()

  set(BREAKPOINT_TEST_NAME test_breakpoint_${ARCH_SUFFIX})
  add_executable(${BREAKPOINT_TEST_NAME} test_breakpoint.c)
  target_compile_options(${BREAKPOINT_TEST_NAME} PRIVATE ${FLAGS})
  target_link_options(${BREAKPOINT_TEST_NAME} PRIVATE ${FLAGS})
  target_include_directories(${BREAKPOINT_TEST_NAME}
                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${BREAKPOINT_TEST_NAME} PRIVATE ${LIB_NAME})
  add_dependencies(${BREAKPOINT_TEST_NAME} ${TARGET_FIXTURE_NAME})
  target_compile_definitions(
    ${BREAKPOINT_TEST_NAME}
    PRIVATE PB_TEST_TARGET_PATH="$<TARGET_FILE:${TARGET_FIXTURE_NAME}>")

  add_test(NAME ${BREAKPOINT_TEST_NAME} COMMAND ${BREAKPOINT_TEST_NAME})
  set_tests_properties(${BREAKPOINT_TEST_NAME} PROPERTIES TIMEOUT 60)
endfunction()

add_native_tests(x64 -m64)

add_native_tests(x86 -m32)
