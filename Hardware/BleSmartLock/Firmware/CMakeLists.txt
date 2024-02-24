cmake_minimum_required(VERSION 3.5)

# Define projects
# ###############################################################################
project(ble_smart_lock
    VERSION 1.0.0
    DESCRIPTION "Bluetooth LE Smart Lock"
    HOMEPAGE_URL "https://github.com/antoine163/ble-smart-lock"
    LANGUAGES C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Enable verbose to debug cmake
# set(CMAKE_VERBOSE_MAKEFILE ON)

# Compilation definitions
# ###############################################################################
add_compile_definitions(
    -DBLUENRG2_DEVICE
    -DBLE_STACK_CONFIGURATION=BLE_STACK_FULL_CONFIGURATION
)

if(MODEL_BLUENRG STREQUAL "M2SP")
    message(STATUS "Build for MODEL:BLUENRG-M2SP.")

    # Additional definitions for BLUENRG-M2SP
    add_compile_definitions(
        -DHS_SPEED_XTAL=HS_SPEED_XTAL_32MHZ
        -DLS_SOURCE=LS_SOURCE_INTERNAL_RO
        -DSMPS_INDUCTOR=SMPS_INDUCTOR_NONE
    )
elseif(MODEL_BLUENRG STREQUAL "M2SA")
    message(STATUS "Build for MODEL:BLUENRG-M2SA.")

    # Additional definitions for BLUENRG-M2SA
    add_compile_definitions(
        -DHS_SPEED_XTAL=HS_SPEED_XTAL_32MHZ
        -DLS_SOURCE=LS_SOURCE_EXTERNAL_32KHZ
        -DSMPS_INDUCTOR=SMPS_INDUCTOR_10uH
    )
else()
    message(FATAL_ERROR "Unsupported BlueNRG module choice ! Please define BLUENRG on 'M2SP' or 'M2SA'")
endif()

# Compilation options
# ###############################################################################
add_compile_options(
    -W -Wall -Wextra -Wpedantic
    -fmessage-length=0

    -ffunction-sections
    -fdata-sections

    -mcpu=cortex-m0
    -mthumb
    -mfloat-abi=soft

    $<$<COMPILE_LANGUAGE:ASM>:-x>
    $<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>
)

# Link options
# ###############################################################################
add_link_options(
    -specs=nano.specs
    # -nostdlib
    # -nodefaultlibs
    # -nostartfiles

    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--cref
    -Wl,--gc-sections

    -mcpu=cortex-m0
    -mthumb
    -mfloat-abi=soft

    -T${CMAKE_SOURCE_DIR}/src/board/linker/BlueNRG2.ld
    -Wl,--print-memory-usage
)

# Add all sources
# ###############################################################################
add_subdirectory(src)

# Output files
# ###############################################################################
add_executable(${CMAKE_PROJECT_NAME}.elf src/app/main.c)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf application)

# Print executable size
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_SIZE_UTIL} ${CMAKE_PROJECT_NAME}.elf
)

# Create hex and bin file
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin
)

add_custom_target(disassemble
    DEPENDS ${CMAKE_PROJECT_NAME}.elf
    COMMAND ${CMAKE_OBJDUMP} -hd ${CMAKE_PROJECT_NAME}.elf > ${CMAKE_PROJECT_NAME}.s
    COMMAND code ${CMAKE_PROJECT_NAME}.s
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)