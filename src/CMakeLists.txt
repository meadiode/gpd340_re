cmake_minimum_required(VERSION 3.12)

# pico-sdk: branch master; sha: f396d05f8252d4670d4ea05c8b7ac938ef0cd381

include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

project(gpd340_reverse C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

add_definitions( )

set(SOURCES
        main.c
)

set (LIBS
        pico_stdlib
        cmsis_core
        hardware_i2c
)

add_executable(gpd340_reverse ${SOURCES})

target_include_directories(gpd340_reverse PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(gpd340_reverse ${LIBS})
pico_add_extra_outputs(gpd340_reverse)
