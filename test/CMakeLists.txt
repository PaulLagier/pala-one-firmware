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
  ${FW_SRC}/pure/hold_gesture.cpp
  ${FW_SRC}/pure/streak_log.cpp
  ${FW_SRC}/storage/click_timings.cpp
  ${FW_SRC}/storage/book_metadata.cpp
  ${FW_SRC}/storage/list_items.cpp
)

set(TEST_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_paths.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_text_util.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hashing.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_paginator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_page_text_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_library_nav.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bookmark_label.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bookmarks_codec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_list_codec.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bookmarks_store.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_list_store.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_app_header.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_hold_gesture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_click_timings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_streak_log.cpp
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

# Exercise the production HAL buffer with a counting File adapter.
add_executable(buffered_file_tests
  main.cpp test_buffered_file_stream.cpp
  ${FW_SRC}/pure/paginator.cpp ${FW_SRC}/pure/text_util.cpp)
target_include_directories(buffered_file_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/native/buffered_file/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME buffered_file_tests COMMAND buffered_file_tests)

# Compile the production storage service and boot screen against deterministic
# filesystem/graphics adapters. No user volume or hardware is needed.
add_executable(storage_recovery_tests
  main.cpp native/storage_recovery/storage.cpp
  ${FW_SRC}/storage/fs_util.cpp ${FW_SRC}/storage/fs_recovery.cpp)
target_include_directories(storage_recovery_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/native/storage_recovery/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME storage_recovery_tests COMMAND storage_recovery_tests)

foreach(locale IN ITEMS EN ES_LA)
  set(recovery_target storage_recovery_ui_${locale})
  add_executable(${recovery_target}
    main.cpp native/storage_recovery_ui/ui.cpp
    ${FW_SRC}/ui/screens/storage_recovery_screen.cpp)
  target_compile_definitions(${recovery_target} PRIVATE LANG_${locale})
  target_include_directories(${recovery_target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/native/storage_recovery_ui/stubs
    ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME ${recovery_target} COMMAND ${recovery_target})
endforeach()

# Production Improv Serial implementation; adapters never access real networks.
add_executable(improv_serial_tests
  main.cpp native/improv_serial/main.cpp
  ${FW_SRC}/third_party/improv/ImprovWiFiLibrary.cpp)
target_compile_definitions(improv_serial_tests PRIVATE ARDUINO ARDUINO_ARCH_ESP32)
target_include_directories(improv_serial_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/native/improv_serial/stubs
  ${FW_SRC}/third_party/improv ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME improv_serial_tests COMMAND improv_serial_tests)

# A system zlib decoder independently checks PBT containers under host sanitizers.
# Firmware uses the pinned ESP32 SDK's bounded miniz implementation instead.
find_package(ZLIB REQUIRED)
add_executable(packed_book_tests main.cpp test_packed_book.cpp
  ${FW_SRC}/pure/paginator.cpp ${FW_SRC}/pure/text_util.cpp)
target_include_directories(packed_book_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(packed_book_tests PRIVATE ZLIB::ZLIB)
add_test(NAME packed_book_tests COMMAND packed_book_tests)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(PACKED_FIXTURES ${CMAKE_CURRENT_BINARY_DIR}/packed-fixtures)
add_custom_command(OUTPUT ${PACKED_FIXTURES}/book.pbt ${PACKED_FIXTURES}/raw.pbt ${PACKED_FIXTURES}/book.txt
  COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_pack_book.py --fixtures ${PACKED_FIXTURES}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_pack_book.py ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/pack_book.py)
add_custom_target(packed_fixtures DEPENDS ${PACKED_FIXTURES}/book.pbt ${PACKED_FIXTURES}/raw.pbt ${PACKED_FIXTURES}/book.txt)
add_dependencies(packed_book_tests packed_fixtures)
target_compile_definitions(packed_book_tests PRIVATE PACKED_FIXTURES="${PACKED_FIXTURES}")
add_test(NAME pack_book_encoder COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_pack_book.py)

add_executable(book_upload_tests main.cpp native/book_upload/upload.cpp
  ${FW_SRC}/web/upload.cpp ${FW_SRC}/lang/join.cpp ${FW_SRC}/pure/text_util.cpp ${FW_SRC}/pure/paths.cpp)
target_compile_definitions(book_upload_tests PRIVATE LANG_EN PACKED_FIXTURES="${PACKED_FIXTURES}")
target_include_directories(book_upload_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/native/book_upload/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(book_upload_tests PRIVATE ZLIB::ZLIB)
add_dependencies(book_upload_tests packed_fixtures)
add_test(NAME book_upload_tests COMMAND book_upload_tests)

add_executable(companion_queue_tests main.cpp test_companion_queue.cpp)
target_include_directories(companion_queue_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME companion_queue_tests COMMAND companion_queue_tests)

# Use the exact firmware JSON release, checksum-verified. Set the standard
# FETCHCONTENT_SOURCE_DIR_ARDUINOJSON option for an offline source checkout.
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
# We only need headers, not the dependency's own project/toolchain options.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
  set(arduinojson_header_only SOURCE_SUBDIR pala-header-only-host-tests)
endif()
FetchContent_Declare(ArduinoJson
  URL https://github.com/bblanchon/ArduinoJson/archive/refs/tags/v7.2.1.tar.gz
  URL_HASH SHA256=2780504927533d64cf4256c57de51412b835b327ef4018c38d862b0664d36d4f
  ${arduinojson_header_only})
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
  FetchContent_MakeAvailable(ArduinoJson)
else()
  FetchContent_GetProperties(ArduinoJson)
  if(NOT arduinojson_POPULATED)
    FetchContent_Populate(ArduinoJson)
  endif()
endif()
find_package(OpenSSL REQUIRED)
add_executable(companion_rpc_tests main.cpp native/companion_rpc/rpc.cpp
  ${FW_SRC}/hal/companion_rpc.cpp ${FW_SRC}/hal/companion_bookmarks.cpp
  native/companion_rpc/runtime_language.cpp ${FW_SRC}/lang/locale_core.cpp
  ${FW_SRC}/lang/locale_data.cpp ${FW_SRC}/pure/catalog_inflater.cpp
  ${FW_SRC}/pure/paths.cpp ${FW_SRC}/pure/hashing.cpp ${FW_SRC}/pure/bookmarks_codec.cpp
  ${FW_SRC}/storage/book_metadata.cpp ${FW_SRC}/storage/click_timings.cpp
  ${FW_SRC}/storage/navigation_profile.cpp)
target_compile_definitions(companion_rpc_tests PRIVATE LANG_EN PALA_COMPANION PALA_RUNTIME_LANG
  ARDUINOJSON_ENABLE_ARDUINO_STRING=1 PACKED_FIXTURES="${PACKED_FIXTURES}")
target_include_directories(companion_rpc_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/native/companion_rpc/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR}
  ${arduinojson_SOURCE_DIR}/src ${FW_SRC})
target_link_libraries(companion_rpc_tests PRIVATE ZLIB::ZLIB OpenSSL::Crypto)
add_dependencies(companion_rpc_tests packed_fixtures)
add_test(NAME companion_rpc_tests COMMAND companion_rpc_tests)

add_executable(companion_ble_tests main.cpp native/companion_ble/ble.cpp
  ${FW_SRC}/hal/companion_ble.cpp)
target_compile_definitions(companion_ble_tests PRIVATE LANG_EN PALA_COMPANION
  ARDUINOJSON_ENABLE_ARDUINO_STRING=1)
target_include_directories(companion_ble_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/native/companion_ble/stubs
  ${CMAKE_CURRENT_SOURCE_DIR}/native/companion_rpc/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR}
  ${arduinojson_SOURCE_DIR}/src ${FW_SRC})
add_test(NAME companion_ble_tests COMMAND companion_ble_tests)

foreach(locale IN ITEMS EN ES_LA)
  set(connection_target companion_ui_${locale})
  add_executable(${connection_target} main.cpp native/companion_ui/ui.cpp
    ${FW_SRC}/ui/screens/upload_screen.cpp)
  target_compile_definitions(${connection_target} PRIVATE LANG_${locale} PALA_COMPANION)
  target_include_directories(${connection_target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/native/companion_ui/stubs
    ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME ${connection_target} COMMAND ${connection_target})
endforeach()

foreach(locale IN ITEMS EN ES_LA FR)
  set(locale_target runtime_locales_${locale})
  add_executable(${locale_target} main.cpp test_runtime_locales.cpp
    ${FW_SRC}/lang/locale_core.cpp ${FW_SRC}/lang/locale_data.cpp ${FW_SRC}/pure/catalog_inflater.cpp)
  target_compile_definitions(${locale_target} PRIVATE LANG_${locale} PALA_RUNTIME_LANG
    CATALOG_DIRECTORY="${FW_SRC}/lang/catalogs")
  target_include_directories(${locale_target} PRIVATE
    ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR} ${arduinojson_SOURCE_DIR}/src)
  target_link_libraries(${locale_target} PRIVATE ZLIB::ZLIB)
  add_test(NAME ${locale_target} COMMAND ${locale_target})
endforeach()
add_test(NAME locale_generator COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/test_locale_generator.py)

# Real pinned U8g2 decoder; only the drawing surface is a host adapter.
add_executable(font_catalog_probe native/font_catalog/glyphs.cpp
  native/font_catalog/decoder/U8g2_for_Adafruit_GFX.cpp ${FW_SRC}/ui/language_fonts.cpp)
target_include_directories(font_catalog_probe PRIVATE native/font_catalog/stubs
  native/font_catalog/decoder ${CMAKE_SOURCE_DIR}/../Pala_One_2_1)
add_test(NAME font_catalog_coverage COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_font_catalog.py $<TARGET_FILE:font_catalog_probe>)

find_program(NODE_EXECUTABLE NAMES node nodejs)
if(NOT NODE_EXECUTABLE)
  message(FATAL_ERROR "Node.js is required for localized browser-script checks")
endif()
add_executable(web_script_probe native/font_catalog/web_scripts.cpp
  ${FW_SRC}/lang/locale_core.cpp ${FW_SRC}/lang/locale_data.cpp
  ${FW_SRC}/pure/catalog_inflater.cpp ${FW_SRC}/pure/web_script.cpp
  ${FW_SRC}/web/localized_scripts.cpp)
target_compile_definitions(web_script_probe PRIVATE LANG_EN PALA_RUNTIME_LANG
  WEB_EDITOR_ASSET="${FW_SRC}/web/assets/editor.js")
target_include_directories(web_script_probe PRIVATE ${CMAKE_SOURCE_DIR}/../Pala_One_2_1)
add_test(NAME localized_web_scripts COMMAND ${Python3_EXECUTABLE} -B
  ${CMAKE_CURRENT_SOURCE_DIR}/test_web_scripts.py $<TARGET_FILE:web_script_probe> ${NODE_EXECUTABLE})
add_test(NAME installer_language COMMAND ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_installer_language.mjs)
add_executable(font_layout_tests main.cpp native/font_catalog/layout.cpp
  ${FW_SRC}/ui/font.cpp ${FW_SRC}/ui/language_fonts.cpp
  ${FW_SRC}/pure/paginator.cpp ${FW_SRC}/pure/text_util.cpp
  native/font_catalog/decoder/U8g2_for_Adafruit_GFX.cpp)
target_compile_definitions(font_layout_tests PRIVATE LANG_EN)
target_include_directories(font_layout_tests PRIVATE native/font_catalog/stubs
  native/font_catalog/decoder ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME font_layout_tests COMMAND font_layout_tests)

# Exercise the actual ISR queue/classifier while a display delays polling.
add_executable(input_frontend_tests native/input_frontend/main.cpp
  ${FW_SRC}/hal/input.cpp ${FW_SRC}/pure/hold_gesture.cpp ${FW_SRC}/storage/click_timings.cpp)
target_compile_definitions(input_frontend_tests PRIVATE LANG_EN)
target_include_directories(input_frontend_tests PRIVATE native/input_frontend/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1)
add_test(NAME input_frontend_tests COMMAND input_frontend_tests)

set(WAKE_SETUP_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/wake-setup.inc)
add_custom_command(OUTPUT ${WAKE_SETUP_INCLUDE}
  COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_SOURCE_DIR}/../scripts/extract_wake_setup.py
    ${CMAKE_SOURCE_DIR}/../Pala_One_2_1/Pala_One_2_1.ino ${WAKE_SETUP_INCLUDE}
  DEPENDS ${CMAKE_SOURCE_DIR}/../Pala_One_2_1/Pala_One_2_1.ino
    ${CMAKE_SOURCE_DIR}/../scripts/extract_wake_setup.py VERBATIM)
add_custom_target(wake_setup_include DEPENDS ${WAKE_SETUP_INCLUDE})
foreach(board IN ITEMS V1_1 V1_2)
  add_executable(wake_setup_${board} native/wake/setup.cpp
    ${FW_SRC}/hal/input.cpp ${FW_SRC}/pure/hold_gesture.cpp ${FW_SRC}/storage/click_timings.cpp)
  add_dependencies(wake_setup_${board} wake_setup_include)
  target_compile_definitions(wake_setup_${board} PRIVATE LANG_EN DISPLAY_${board})
  target_include_directories(wake_setup_${board} PRIVATE native/input_frontend/stubs
    ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_BINARY_DIR})
  add_test(NAME wake_setup_${board} COMMAND wake_setup_${board})
endforeach()

add_executable(navigation_profile_tests main.cpp native/navigation_profile/main.cpp
  ${FW_SRC}/storage/navigation_profile.cpp)
target_compile_definitions(navigation_profile_tests PRIVATE LANG_EN)
target_include_directories(navigation_profile_tests PRIVATE native/navigation_profile/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME navigation_profile_tests COMMAND navigation_profile_tests)

add_test(NAME compressed_web_assets COMMAND ${Python3_EXECUTABLE} -B
  ${CMAKE_CURRENT_SOURCE_DIR}/test_web_assets.py)

add_test(NAME reader_window_integration COMMAND ${Python3_EXECUTABLE} -B
  ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/test_reader_window.py
  --firmware ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(factory_reset_tests main.cpp native/factory_reset/main.cpp
  ${FW_SRC}/storage/factory_reset.cpp)
target_include_directories(factory_reset_tests PRIVATE native/factory_reset/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME factory_reset_tests COMMAND factory_reset_tests)

add_test(NAME release_packaging COMMAND ${Python3_EXECUTABLE} -B
  ${CMAKE_CURRENT_SOURCE_DIR}/test_release_package.py)

add_test(NAME browser_read_search COMMAND ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_read_search.mjs $<TARGET_FILE:web_script_probe>)

# Persisted custom lock bindings use the production settings and lock modules.
add_executable(lock_gesture_tests main.cpp native/lock/main.cpp
  ${FW_SRC}/ui/lock.cpp ${FW_SRC}/ui/reader_actions.cpp)
target_include_directories(lock_gesture_tests PRIVATE native/lock/stubs
  ${CMAKE_SOURCE_DIR}/../Pala_One_2_1 ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME lock_gesture_tests COMMAND lock_gesture_tests)

add_test(NAME browser_image_editor COMMAND ${NODE_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/test_editor.mjs $<TARGET_FILE:web_script_probe>)
