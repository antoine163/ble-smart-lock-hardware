# Add application library
add_library(application STATIC)

target_sources(application PRIVATE
)

target_sources(application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ble_events.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syscall.c
)

target_include_directories(application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(application PUBLIC
    board
)
