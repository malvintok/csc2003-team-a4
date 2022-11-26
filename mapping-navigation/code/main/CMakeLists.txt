add_executable(main main.c
        )

# pull in common dependencies
target_link_libraries(main pico_stdlib)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)

# add url via pico_set_program_url
# example_auto_set_url(blink)
