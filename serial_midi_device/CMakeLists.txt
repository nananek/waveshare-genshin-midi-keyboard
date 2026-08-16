# ===========================================================================
#  serial_midi_device — UART(31250, 8N1) RX → native USB = USB-MIDI デバイス
#  ルート CMakeLists.txt が pico_sdk_import / pico_sdk_init() を済ませている
#  ので、ここでは pico_sdk_import.cmake を include しない (二重 init 防止)。
#  PICO_TINYUSB_PATH はルートで upstream 0.20.0 に設定済み。
# ===========================================================================

set(EXEC serial_midi_device)

add_executable(${EXEC}
    main.c
    usb_descriptors.c
)

# 自身のディレクトリの tusb_config.h / config.h を解決する (src/ は含めない:
# src/tusb_config.h を拾うと HID 用設定になり衝突する)
target_include_directories(${EXEC} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${EXEC} PRIVATE
    pico_stdlib
    tinyusb_device        # SDK の INTERFACE ライブラリ。midi_device.c を含む
)

# デバッグログ: UART0 (GP0/GP1) @115200。native USB は MIDI が占有するので stdio-usb 不可。
pico_enable_stdio_uart(${EXEC} 1)
pico_enable_stdio_usb(${EXEC} 0)

pico_add_extra_outputs(${EXEC})