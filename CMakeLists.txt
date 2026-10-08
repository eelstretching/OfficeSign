# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# The sign uses Bluetooth through the CYW43 radio, so it is built for the Pico 2 W.
set(PICO_BOARD pico2_w CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(OfficeSignBuild
  VERSION 0.1
  DESCRIPTION "Office sign firmware"
  LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_CXX_ENABLE_EXCEPTIONS 0)

pico_sdk_init()

if(PICO_SDK_VERSION_STRING VERSION_LESS "2.3.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK 2.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/PicoLEDs/src/CMakeLists.txt)
  message(FATAL_ERROR "The PicoLEDs submodule is missing. Run: git submodule update --init")
endif()

# The PicoLEDs library, from the submodule (just the library, not its examples).
add_subdirectory(PicoLEDs/src PicoLEDs)

add_subdirectory(firmware)
