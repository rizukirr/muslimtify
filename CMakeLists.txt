cmake_minimum_required(VERSION 3.22)
project(muslimtify LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
pkg_check_modules(LIBCURL REQUIRED libcurl)

add_executable(muslimtify
    src/muslimtify.c
    src/config.c
    src/location.c
    src/prayer_checker.c
    src/notification.c
    src/display.c
    src/cli.c
)

target_include_directories(muslimtify PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBNOTIFY_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
)

target_link_libraries(muslimtify
    ${LIBNOTIFY_LIBRARIES}
    ${LIBCURL_LIBRARIES}
    m
)

target_compile_options(muslimtify PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Installation
install(TARGETS muslimtify DESTINATION bin)

# Install icon to multiple locations for maximum compatibility
install(FILES assets/muslimtify.png
        DESTINATION share/icons/hicolor/128x128/apps/)
install(FILES assets/muslimtify.png
        DESTINATION share/pixmaps/)
