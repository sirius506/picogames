cmake_minimum_required(VERSION 3.12)

set(NAME picogames)
set(PICO_BOARD "pico_w")
set(PICO_PLATFORM "rp2040")
#set(PICO_BOARD "pico2_w")
#set(PICO_PLATFORM "rp2350")
set(PICO_STDIO_USB 1)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(picogames C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_subdirectory(lvgl)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
	src/XPT2046.c
        src/main.c
	src/hakoirimusume.c
	src/hakomusu_image.c
	src/menu.c
	src/pegsolitaire.c
	src/invaderpico.c
	src/spi-lcdpacman.c
	src/pacman2data.c
	src/tetrispico.c
        src/tetrisfont.c
	src/ili9341_spi.c
	src/graphlib.c
	src/character.c
	src/picogames.c
	src/graphlib.h
	src/LCDdriver.h
	src/gamepad.c
	src/hid_host_gamepad.c
	src/8bitdomicro.c
	src/8bitdozero2.c
	src/dualsense.c
	src/dualshock4.c
	src/bluetooth_black.c
	src/bluetooth_scan_black.c
	src/bluetooth_scan_blue.c
)

# Pull in basic dependencies
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_stdlib
  pico_btstack_classic
  pico_btstack_cyw43
  pico_cyw43_arch_none
  pico_multicore
  hardware_spi hardware_pwm
  lvgl
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})
