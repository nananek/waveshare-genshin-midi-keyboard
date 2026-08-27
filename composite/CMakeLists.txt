# ===========================================================================
#  genshin_midi_kbd_composite — 1枚化 (HID+MIDI composite) ファーム
#  serial_midi_device/CMakeLists.txt を雛形に、src/ の純粋ロジックを共用。
#  ルート CMakeLists.txt が pico_sdk_import / pico_sdk_init() を済ませている
#  ので、ここでは include しない (二重 init 防止)。
# ===========================================================================

set(EXEC genshin_midi_kbd_composite)

add_executable(${EXEC}
    main.c
    usb_descriptors.c
    midi_bridge.c
    reset_button.c
    sustain_switch.c
    ${CMAKE_SOURCE_DIR}/src/debounced_switch.c
    ${CMAKE_SOURCE_DIR}/src/hid_device.c
    ${CMAKE_SOURCE_DIR}/src/hid_mute.c
    ${CMAKE_SOURCE_DIR}/src/midi_host.c
    ${CMAKE_SOURCE_DIR}/src/midi_parse.c
    ${CMAKE_SOURCE_DIR}/src/midi_note_filter.c
    ${CMAKE_SOURCE_DIR}/src/mirror_filter_switch.c
    ${CMAKE_SOURCE_DIR}/src/note_mapper.c
    ${CMAKE_SOURCE_DIR}/src/nkro_report.c
    ${PICO_TINYUSB_PATH}/src/class/midi/midi_host.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

set_target_properties(${EXEC} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(${EXEC} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(${EXEC} PRIVATE
    PIO_USB_USE_TINYUSB
    COMPOSITE_MIDI=1
)

target_link_libraries(${EXEC} PRIVATE
    pico_stdlib
    pico_multicore
    pico_unique_id
    hardware_pio
    hardware_dma
    tinyusb_device
    tinyusb_host
    pico_pio_usb
)

pico_enable_stdio_uart(${EXEC} 1)
pico_enable_stdio_usb(${EXEC} 0)

pico_add_extra_outputs(${EXEC})
