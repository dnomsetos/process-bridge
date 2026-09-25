add_library(pb-log STATIC src/log.c)

target_include_directories(pb-log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
