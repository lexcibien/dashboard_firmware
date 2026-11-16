# Firmware para Dashboard (ESP32-C3)

cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(dashboard_firmware DESCRIPTION Firmware para o dashboard com ESP32-C3 que contém um display TFT)

# TFT_eSPI 2.5.44 contém macros do ESP32-C3 com indentação que o toolchain
# atual transforma em erro. O alvo já existe neste ponto do CMake do projeto;
# mantemos o submódulo sem alterações e relaxamos somente esse diagnóstico.
idf_component_get_property(TFT_ESPI_COMPONENT_LIB TFT_eSPI COMPONENT_LIB)
if(TFT_ESPI_COMPONENT_LIB)
    target_compile_options(${TFT_ESPI_COMPONENT_LIB} PRIVATE
        -Wno-error=misleading-indentation
    )
endif()
