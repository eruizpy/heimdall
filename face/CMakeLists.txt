# Heimdall face: display + touch terminal for the ESP32-2432S028 (CYD2USB).
# Separate ESP-IDF project; talks to the robot core over UART (docs/heimdall/face-protocol.md).
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(heimdall-face)
