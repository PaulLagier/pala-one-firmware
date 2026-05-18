cmake_minimum_required(VERSION 3.16)
project(pala_web_emu CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)

set(SKETCH_ROOT ${CMAKE_SOURCE_DIR}/../Pala_One_2_1)
set(FW_SRC      ${SKETCH_ROOT}/src)
set(TEST_ROOT   ${CMAKE_SOURCE_DIR}/../test)
set(STUBS_ROOT  ${CMAKE_SOURCE_DIR}/stubs)

# ---------------------------------------------------------------------------
#  Pure modules — identical to the test build
# ---------------------------------------------------------------------------
set(PURE_SRC
  ${FW_SRC}/pure/paths.cpp
  ${FW_SRC}/pure/text_util.cpp
  ${FW_SRC}/pure/find_text.cpp
  ${FW_SRC}/pure/hashing.cpp
  ${FW_SRC}/pure/paginator.cpp
  ${FW_SRC}/pure/library_nav.cpp
  ${FW_SRC}/pure/bookmark_label.cpp
  ${FW_SRC}/pure/bookmarks_codec.cpp
  ${FW_SRC}/pure/list_codec.cpp
  ${FW_SRC}/pure/app_header.cpp
)

# ---------------------------------------------------------------------------
#  Storage modules — pure API compiles; firmware glue (#ifdef ARDUINO) skipped
# ---------------------------------------------------------------------------
set(STORAGE_SRC
  ${FW_SRC}/storage/book_metadata.cpp
  ${FW_SRC}/storage/list_items.cpp
  ${FW_SRC}/storage/library.cpp
  ${FW_SRC}/storage/app_catalog.cpp
)

# ---------------------------------------------------------------------------
#  Web handlers — the actual code under development.
#  upload.cpp / apps_upload.cpp omitted (streaming upload, no GET pages).
# ---------------------------------------------------------------------------
set(WEB_SRC
  ${FW_SRC}/web/web.cpp
  ${FW_SRC}/web/chrome.cpp
  ${FW_SRC}/web/files.cpp
  ${FW_SRC}/web/bookmarks.cpp
  ${FW_SRC}/web/list.cpp
  ${FW_SRC}/web/settings.cpp
  ${FW_SRC}/web/reset.cpp
)

# ---------------------------------------------------------------------------
#  Stubs — Arduino / ESP32 API replacements for the host build
# ---------------------------------------------------------------------------
set(STUB_SRC
  ${STUBS_ROOT}/web_server.cpp
  ${STUBS_ROOT}/fs_stub.cpp
  ${STUBS_ROOT}/state.cpp
  ${STUBS_ROOT}/ui_stubs.cpp
  ${STUBS_ROOT}/text_stubs.cpp
  ${STUBS_ROOT}/book_metadata_glue.cpp
  ${STUBS_ROOT}/list_items_glue.cpp
  ${STUBS_ROOT}/fs_util_stub.cpp
  ${STUBS_ROOT}/upload_stub.cpp
)

add_executable(pala_web_emu
  ${CMAKE_SOURCE_DIR}/main.cpp
  ${CMAKE_SOURCE_DIR}/mock_data.cpp
  ${PURE_SRC}
  ${STORAGE_SRC}
  ${WEB_SRC}
  ${STUB_SRC}
)

target_include_directories(pala_web_emu PRIVATE
  # MUST be first — these shadow real Arduino/firmware headers
  ${STUBS_ROOT}/overrides
  # dev/ root — resolves #include "stubs/..." paths from override headers
  ${CMAKE_SOURCE_DIR}
  # Firmware sketch root — resolves #include "src/..." paths
  ${SKETCH_ROOT}
  # Firmware src/ root — resolves #include "storage/..." paths (used by map_kv_store.h)
  ${FW_SRC}
  # Test utilities — provides map_kv_store.h
  ${TEST_ROOT}
)

target_link_libraries(pala_web_emu PRIVATE httplib::httplib)

# PROGMEM is a no-op on the host (it's a flash-storage hint for AVR/Xtensa).
target_compile_definitions(pala_web_emu PRIVATE PROGMEM=)

target_compile_options(pala_web_emu PRIVATE
  -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
  # Pre-include system headers before arduino_compat.h can define min/max macros.
  -include ${CMAKE_SOURCE_DIR}/stubs/prelude.h
)
