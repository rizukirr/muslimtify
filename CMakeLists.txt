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

# Testing
enable_testing()

add_executable(test_prayertimes tests/test_prayertimes.c)
target_include_directories(test_prayertimes PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_prayertimes m)
target_compile_options(test_prayertimes PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
set_target_properties(test_prayertimes PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(NAME prayertimes COMMAND test_prayertimes)

add_executable(test_cli
    tests/test_cli.c
    src/cli.c
    src/config.c
    src/location.c
    src/prayer_checker.c
    src/notification.c
    src/display.c
)
target_include_directories(test_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIBNOTIFY_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
)
target_link_libraries(test_cli ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
target_compile_options(test_cli PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(test_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
add_test(NAME cli COMMAND test_cli)

add_executable(test_prayer_checker
    tests/test_prayer_checker.c
    src/prayer_checker.c
    src/config.c
)
target_include_directories(test_prayer_checker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_prayer_checker m)
target_compile_options(test_prayer_checker PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(test_prayer_checker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
add_test(NAME prayer_checker COMMAND test_prayer_checker)

add_executable(test_config
    tests/test_config.c
    src/config.c
)
target_include_directories(test_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_config m)
target_compile_options(test_config PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(test_config PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
add_test(NAME config COMMAND test_config)

# Installation
install(TARGETS muslimtify DESTINATION bin)

# Install icon to multiple locations for maximum compatibility
install(FILES assets/muslimtify.png
        DESTINATION share/icons/hicolor/128x128/apps/)
install(FILES assets/muslimtify.png
        DESTINATION share/pixmaps/)
