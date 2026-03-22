cmake_minimum_required(VERSION 3.22...3.31)
project(muslimtify VERSION 0.2.0 LANGUAGES C)

# ── Default build type ────────────────────────────────────────────────────────

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(FULL_PROJECT_VERSION "${PROJECT_VERSION}-rc.1")

# ── C standard and output dirs ────────────────────────────────────────────────

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(GNUInstallDirs)

# ── Dependencies ──────────────────────────────────────────────────────────────

if(WIN32)
    include(FetchContent)
    FetchContent_Declare(curl
        URL https://github.com/curl/curl/releases/download/curl-8_11_1/curl-8.11.1.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(curl)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
    pkg_check_modules(LIBCURL REQUIRED libcurl)
endif()

# ── Generated version header ─────────────────────────────────────────────────

configure_file(src/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h @ONLY)

# ── Helper function ──────────────────────────────────────────────────────────

function(muslimtify_set_target_defaults target)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wformat=2
        )
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

# ── Object library (shared app sources) ──────────────────────────────────────

add_library(muslimtify_lib OBJECT
    src/cli.c
    src/cmd_show.c
    src/cmd_next.c
    src/cmd_config.c
    src/cmd_location.c
    src/cmd_prayer.c
    $<IF:$<BOOL:${WIN32}>,src/cmd_daemon_win.c,src/cmd_daemon.c>
    src/config.c
    src/cache.c
    src/location.c
    src/prayer_checker.c
    $<IF:$<BOOL:${WIN32}>,src/notification_win.c,src/notification.c>
    src/display.c
    $<IF:$<BOOL:${WIN32}>,src/platform_win.c,src/platform_linux.c>
)
muslimtify_set_target_defaults(muslimtify_lib)
if(WIN32)
    target_include_directories(muslimtify_lib PRIVATE ${curl_SOURCE_DIR}/include)
else()
    target_include_directories(muslimtify_lib PRIVATE
        ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
endif()

# ── Main executable ──────────────────────────────────────────────────────────

add_executable(muslimtify src/muslimtify.c)
muslimtify_set_target_defaults(muslimtify)
if(WIN32)
    target_link_libraries(muslimtify
        muslimtify_lib ole32 runtimeobject CURL::libcurl)
else()
    target_include_directories(muslimtify PRIVATE
        ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
    target_link_libraries(muslimtify
        muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
endif()

# ── Testing ──────────────────────────────────────────────────────────────────

option(BUILD_TESTING "Build test targets" ON)

if(BUILD_TESTING)
    enable_testing()

    if(NOT WIN32)
        add_executable(test_cli tests/test_cli.c)
        muslimtify_set_target_defaults(test_cli)
        target_include_directories(test_cli PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
        target_link_libraries(test_cli muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
        add_test(NAME cli COMMAND test_cli)

        add_executable(test_prayer_checker tests/test_prayer_checker.c)
        muslimtify_set_target_defaults(test_prayer_checker)
        target_include_directories(test_prayer_checker PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
        target_link_libraries(test_prayer_checker muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
        add_test(NAME prayer_checker COMMAND test_prayer_checker)

        add_executable(test_config tests/test_config.c)
        muslimtify_set_target_defaults(test_config)
        target_include_directories(test_config PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
        target_link_libraries(test_config muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
        add_test(NAME config COMMAND test_config)

        add_executable(test_cache tests/test_cache.c)
        muslimtify_set_target_defaults(test_cache)
        target_include_directories(test_cache PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
        target_link_libraries(test_cache muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
        add_test(NAME cache COMMAND test_cache)
    endif()

    # Cross-platform tests (no POSIX dependencies)
    add_executable(test_prayertimes tests/test_prayertimes.c)
    muslimtify_set_target_defaults(test_prayertimes)
    if(NOT WIN32)
      target_link_libraries(test_prayertimes m)
    endif()
    add_test(NAME prayertimes COMMAND test_prayertimes)

    add_executable(test_json tests/test_json.c)
    muslimtify_set_target_defaults(test_json)
    if(NOT WIN32)
      target_link_libraries(test_json m)
    endif()
    add_test(NAME json COMMAND test_json)
endif()

# ── Install ──────────────────────────────────────────────────────────────────

install(TARGETS muslimtify DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES assets/muslimtify.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/)
install(FILES assets/muslimtify.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps/)

install(FILES systemd/muslimtify.service systemd/muslimtify.timer
    DESTINATION lib/systemd/user/)
