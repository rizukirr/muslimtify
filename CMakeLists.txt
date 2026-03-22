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
    # Notification uses WinRT — linked directly, no pkg-config needed
    # libcurl Windows integration is out of scope for this plan
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
    src/cmd_daemon.c
    src/config.c
    src/location.c
    src/prayer_checker.c
    $<IF:$<BOOL:${WIN32}>,src/notification_win.c,src/notification.c>
    src/display.c
)
muslimtify_set_target_defaults(muslimtify_lib)
if(NOT WIN32)
    target_include_directories(muslimtify_lib PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
endif()
target_include_directories(muslimtify_lib PRIVATE ${LIBCURL_INCLUDE_DIRS})

# ── Main executable ──────────────────────────────────────────────────────────

add_executable(muslimtify src/muslimtify.c)
muslimtify_set_target_defaults(muslimtify)
if(NOT WIN32)
    target_include_directories(muslimtify PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
endif()
target_include_directories(muslimtify PRIVATE ${LIBCURL_INCLUDE_DIRS})
target_link_libraries(muslimtify
    muslimtify_lib
    $<$<BOOL:${WIN32}>:ole32>
    $<$<BOOL:${WIN32}>:runtimeobject>
    $<$<NOT:$<BOOL:${WIN32}>>:${LIBNOTIFY_LIBRARIES}>
    ${LIBCURL_LIBRARIES}
    m
)

# ── Testing ──────────────────────────────────────────────────────────────────

option(BUILD_TESTING "Build test targets" ON)

if(BUILD_TESTING)
    enable_testing()

    add_executable(test_prayertimes tests/test_prayertimes.c)
    muslimtify_set_target_defaults(test_prayertimes)
    target_link_libraries(test_prayertimes m)
    add_test(NAME prayertimes COMMAND test_prayertimes)

    add_executable(test_cli tests/test_cli.c)
    muslimtify_set_target_defaults(test_cli)
    if(NOT WIN32)
        target_include_directories(test_cli PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
    endif()
    target_include_directories(test_cli PRIVATE ${LIBCURL_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(test_cli muslimtify_lib ole32 runtimeobject ${LIBCURL_LIBRARIES} m)
    else()
        target_link_libraries(test_cli muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    endif()
    add_test(NAME cli COMMAND test_cli)

    add_executable(test_prayer_checker tests/test_prayer_checker.c)
    muslimtify_set_target_defaults(test_prayer_checker)
    if(NOT WIN32)
        target_include_directories(test_prayer_checker PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
    endif()
    target_include_directories(test_prayer_checker PRIVATE ${LIBCURL_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(test_prayer_checker muslimtify_lib ole32 runtimeobject ${LIBCURL_LIBRARIES} m)
    else()
        target_link_libraries(test_prayer_checker muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    endif()
    add_test(NAME prayer_checker COMMAND test_prayer_checker)

    add_executable(test_config tests/test_config.c)
    muslimtify_set_target_defaults(test_config)
    if(NOT WIN32)
        target_include_directories(test_config PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})
    endif()
    target_include_directories(test_config PRIVATE ${LIBCURL_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(test_config muslimtify_lib ole32 runtimeobject ${LIBCURL_LIBRARIES} m)
    else()
        target_link_libraries(test_config muslimtify_lib ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
    endif()
    add_test(NAME config COMMAND test_config)

    add_executable(test_json tests/test_json.c)
    muslimtify_set_target_defaults(test_json)
    target_link_libraries(test_json m)
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
