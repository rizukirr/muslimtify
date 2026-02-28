cmake_minimum_required(VERSION 3.22)
project(muslimtify VERSION 0.1.3 LANGUAGES C)

# ── Default build type ────────────────────────────────────────────────────────

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(FULL_PROJECT_VERSION "${PROJECT_VERSION}")

# ── C standard and output dirs ────────────────────────────────────────────────

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(GNUInstallDirs)

# ── Dependencies ──────────────────────────────────────────────────────────────

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
pkg_check_modules(LIBCURL REQUIRED libcurl)

# ── Generated version header ─────────────────────────────────────────────────

configure_file(src/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h @ONLY)

# ── Helper function ──────────────────────────────────────────────────────────

function(muslimtify_set_target_defaults target)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
    )
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endfunction()

# ── Object library (shared app sources) ──────────────────────────────────────

add_library(muslimtify_lib OBJECT
    src/cli.c
    src/config.c
    src/location.c
    src/prayer_checker.c
    src/notification.c
    src/display.c
)
muslimtify_set_target_defaults(muslimtify_lib)
target_include_directories(muslimtify_lib PRIVATE
    ${LIBNOTIFY_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
)

# ── Main executable ──────────────────────────────────────────────────────────

add_executable(muslimtify src/muslimtify.c)
muslimtify_set_target_defaults(muslimtify)
target_include_directories(muslimtify PRIVATE
    ${LIBNOTIFY_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
)
target_link_libraries(muslimtify
    muslimtify_lib
    ${LIBNOTIFY_LIBRARIES}
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
    target_include_directories(test_cli PRIVATE
        ${LIBNOTIFY_INCLUDE_DIRS}
        ${LIBCURL_INCLUDE_DIRS}
    )
    target_link_libraries(test_cli
        muslimtify_lib
        ${LIBNOTIFY_LIBRARIES}
        ${LIBCURL_LIBRARIES}
        m
    )
    add_test(NAME cli COMMAND test_cli)

    add_executable(test_prayer_checker
        tests/test_prayer_checker.c
        src/prayer_checker.c
        src/config.c
    )
    muslimtify_set_target_defaults(test_prayer_checker)
    target_link_libraries(test_prayer_checker m)
    add_test(NAME prayer_checker COMMAND test_prayer_checker)

    add_executable(test_config
        tests/test_config.c
        src/config.c
    )
    muslimtify_set_target_defaults(test_config)
    target_link_libraries(test_config m)
    add_test(NAME config COMMAND test_config)
endif()

# ── Install ──────────────────────────────────────────────────────────────────

install(TARGETS muslimtify DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES assets/muslimtify.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/)
install(FILES assets/muslimtify.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps/)

install(FILES systemd/muslimtify.service systemd/muslimtify.timer
    DESTINATION lib/systemd/user/)
