add_executable(main main.c
        )

# pull in common dependencies
target_link_libraries(main pico_stdlib hardware_irq hardware_gpio hardware_uart hardware_timer hardware_adc hardware_pwm)

pico_enable_stdio_usb(main 1)
# pico_enable_stdio_uart(main 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)

