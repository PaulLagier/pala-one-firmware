cmake_minimum_required(VERSION 3.13)
project(pala_one_host_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Source modules under test
#
# Pure modules — must compile cleanly without Arduino.h.
# KV-store-backed modules — also host-compatible (their firmware-only code
# is inside `#ifdef ARDUINO` blocks).
# ---------------------------------------------------------------------------
set(FW_SRC ${CMAKE_SOURCE_DIR}/../Pala_One_2_1/src)
set(SUT_SRC
  ${FW_SRC}/pure/paths.cpp
  ${FW_SRC}/pure/text_util.cpp
  ${FW_SRC}/pure/hashing.cpp
  ${FW_SRC}/pure/paginator.cpp
  ${FW_SRC}/pure/library_nav.cpp
  ${FW_SRC}/pure/bookmark_label.cpp
  ${FW_SRC}/pure/bookmarks_codec.cpp
  ${FW_SRC}/pure/list_codec.cpp
  ${FW_SRC}/pure/app_header.cpp
  ${FW_SRC}/storage/book_metadata.cpp
  ${FW_SRC}/storage/list_items.cpp
)

set(TEST_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_paths.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_text_util.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hashing.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_paginator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_library_nav.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bookmark_label.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bookmarks_codec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_list_codec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bookmarks_store.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_list_store.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_app_header.cpp
)

add_executable(host_tests ${SUT_SRC} ${TEST_SRC})
target_include_directories(host_tests PRIVATE
  ${FW_SRC}
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1   # for pala_api.h / pala_app.h
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
  # /W4 with two suppressions:
  #   C4310 — "cast truncates constant value" — deliberate when building
  #           raw UTF-8 byte arrays in tests like `(char)0xC3`.
  target_compile_options(host_tests PRIVATE /W4 /permissive- /wd4310)
  # Silence "use strcpy_s / sprintf_s" CRT-deprecation warnings — those
  # functions are deliberate, and we're targeting C++17 portably.
  target_compile_definitions(host_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(host_tests PRIVATE -Wall -Wextra -Wpedantic
                                            -Wno-unused-function)
endif()

enable_testing()
add_test(NAME host_tests COMMAND host_tests)
