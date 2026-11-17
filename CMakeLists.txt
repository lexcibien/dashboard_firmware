# Firmware para Dashboard (ESP32-C3)

cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ENV{KCONFIG_REPORT_VERBOSITY} "quiet")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(dashboard_firmware DESCRIPTION Firmware para o dashboard com ESP32-C3 que contém um display TFT)
