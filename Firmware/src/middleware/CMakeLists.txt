# Add middleware library
add_library(middleware INTERFACE)

# Add freeRTOS library to middleware library library
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/src/app)
set(FREERTOS_PORT "GCC_ARM_CM0" CACHE STRING "" FORCE)
add_subdirectory(FreeRTOS-Kernel)

target_link_libraries(middleware INTERFACE
    freertos_kernel
)