add_library(pb-log STATIC src/log.c)

set_target_properties(pb-log PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(pb-log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
