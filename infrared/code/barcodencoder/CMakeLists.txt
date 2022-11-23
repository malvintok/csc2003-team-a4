add_executable(barcodencoder
        barcodencoder.c
        )

# pull in common dependencies
target_link_libraries(barcodencoder pico_stdlib hardware_irq hardware_uart hardware_timer hardware_adc)

pico_enable_stdio_usb(barcodencoder 1)
pico_enable_stdio_uart(barcodencoder 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(barcodencoder)

# add url via pico_set_program_url
example_auto_set_url(barcodencoder)

