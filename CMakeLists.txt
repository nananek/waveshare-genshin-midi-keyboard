cmake_minimum_required(VERSION 3.13)

# ===========================================================================
#  PIO-MIDI gadget + CH9329 keyboard (RP2350A)
#
#  [MIDI keyboard] --USB--> PIO-USB host --+
#                                          +--> native USB = USB-MIDI gadget
#                                          +--> UART1 9600 --> CH9329 --> PC HID
# ===========================================================================

# ---- ターゲット MCU: RP2350A ----
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2 CACHE STRING "Target board (RP2350A)")
endif()

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})

# ---------------------------------------------------------------------------
#  依存ライブラリのパス (scripts/fetch_deps.sh が lib/ 以下へ配置)
# ---------------------------------------------------------------------------
# MIDI ホスト対応の upstream TinyUSB。
#   ※ pico-sdk 同梱の raspberrypi/tinyusb には midi_host が無く tuh_midi_* を
#     リンクできないため、upstream 0.20.0+ を指す必要がある。
if(NOT DEFINED PICO_TINYUSB_PATH)
    set(PICO_TINYUSB_PATH ${PROJECT_ROOT}/lib/tinyusb)
endif()

# Pico-PIO-USB (sekigon-gonnoc)
if(NOT DEFINED PICO_PIO_USB_PATH)
    set(PICO_PIO_USB_PATH ${PROJECT_ROOT}/lib/Pico-PIO-USB)
endif()

# ---- pico-sdk 取り込み ----
include(pico_sdk_import.cmake)

project(midi_ch9329_gadget C CXX ASM)

# 依存が揃っているか早期チェック
if(NOT EXISTS ${PICO_TINYUSB_PATH}/src/class/midi/midi_host.c)
    message(FATAL_ERROR
        "MIDI ホスト対応の TinyUSB が ${PICO_TINYUSB_PATH} に見つかりません。\n"
        "  先に `scripts/fetch_deps.sh` を実行してください。")
endif()
if(NOT EXISTS ${PICO_PIO_USB_PATH}/src/pio_usb.c)
    message(FATAL_ERROR
        "Pico-PIO-USB が ${PICO_PIO_USB_PATH} に見つかりません。\n"
        "  先に `scripts/fetch_deps.sh` を実行してください。")
endif()

pico_sdk_init()

# Pico-PIO-USB (INTERFACE ライブラリ target 名: pico_pio_usb)
add_subdirectory(${PICO_PIO_USB_PATH} pico_pio_usb)

add_executable(${PROJECT_NAME}
    src/main.c
    src/usb_descriptors.c
    src/debounced_switch.c
    src/hid_mute.c
    src/midi_bridge.c
    src/midi_host.c
    src/midi_parse.c
    src/mirror_filter_switch.c
    src/note_mapper.c
    src/ch9329_keyboard.c
    src/ch9329_protocol.c
    src/ch9329_uart.c

    # --- SDK の tinyusb_host が列挙しないソースを手動追加 ---
    ${PICO_TINYUSB_PATH}/src/class/midi/midi_host.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_ROOT}/src)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PIO_USB_USE_TINYUSB
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    pico_multicore
    pico_unique_id
    hardware_pio
    hardware_dma
    tinyusb_device
    tinyusb_host
    pico_pio_usb
)

# デバッグ出力は UART0 へ。native USB は USB-MIDI が占有する。
pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})
