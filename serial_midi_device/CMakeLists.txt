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

# 成果物 (elf/uf2/bin/hex/dis/map) を build/ 直下に置く。
# サブディレクトリ target は既定では build/serial_midi_device/ に生成され、
# scripts/build_docker.sh や README/CLAUDE.md の記載パス (build/serial_midi_device.uf2) と
# 食い違うため、明示的に揃える。
#   - .uf2 は ARCHIVE_OUTPUT_DIRECTORY に従う (pico-sdk tools/CMakeLists.txt)
#   - .elf/.bin/.hex/.dis/.map は RUNTIME_OUTPUT_DIRECTORY に従う
set_target_properties(${EXEC} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 自身のディレクトリの tusb_config.h / config.h を解決する (src/ は含めない:
# src/tusb_config.h を拾うと HID 用設定になり衝突する)
target_include_directories(${EXEC} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${EXEC} PRIVATE
    pico_stdlib
    pico_unique_id        # usb_descriptors.c が pico_get_unique_board_id_string を使用
    tinyusb_device        # SDK の INTERFACE ライブラリ。midi_device.c を含む
)

# デバッグログ: UART0 (GP0/GP1) @115200。native USB は MIDI が占有するので stdio-usb 不可。
pico_enable_stdio_uart(${EXEC} 1)
pico_enable_stdio_usb(${EXEC} 0)

pico_add_extra_outputs(${EXEC})
