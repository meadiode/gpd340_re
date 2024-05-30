cmake_minimum_required(VERSION 3.12)

# pico-sdk: branch master; sha: f396d05f8252d4670d4ea05c8b7ac938ef0cd381

include(pico_sdk_import.cmake)

project(gpd340_reverse C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()


set (LIBS
        pico_stdlib
        pico_multicore
        cmsis_core
        hardware_i2c
        hardware_pwm
)

add_executable(bruteforcer bruteforcer.c)

target_include_directories(bruteforcer PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(bruteforcer ${LIBS})
pico_add_extra_outputs(bruteforcer)


add_executable(demo demo.c)

target_include_directories(demo PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(demo ${LIBS})
pico_add_extra_outputs(demo)
