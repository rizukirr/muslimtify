# GUI target. Entry: src/muslimtify_gui.c. Component sources added under src/gui/
# as the GUI grows; link muslimtify_core for prayer logic, config, cache, etc.

add_executable(muslimtify-gui ${CMAKE_SOURCE_DIR}/src/muslimtify_gui.c)
muslimtify_set_target_defaults(muslimtify-gui)

if(WIN32)
    target_include_directories(muslimtify-gui PRIVATE ${curl_SOURCE_DIR}/include)
    target_link_libraries(muslimtify-gui PRIVATE
        muslimtify_core ccompose ole32 runtimeobject CURL::libcurl)
else()
    target_include_directories(muslimtify-gui PRIVATE
        ${LIBNOTIFY_INCLUDE_DIRS} ${LIBCURL_INCLUDE_DIRS})
    target_link_libraries(muslimtify-gui PRIVATE
        muslimtify_core ccompose ${LIBNOTIFY_LIBRARIES} ${LIBCURL_LIBRARIES} m)
endif()

install(TARGETS muslimtify-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
