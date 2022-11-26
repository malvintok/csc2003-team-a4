add_executable(motor
        motor.c
        )

# pull in common dependencies
target_link_libraries(motor pico_stdlib hardware_gpio hardware_clocks hardware_pwm hardware_timer hardware_pio)

pico_enable_stdio_uart(motor 0)
pico_enable_stdio_usb(motor 1)

# create map/bin/hex file etc. (to get uf2 file)
pico_add_extra_outputs(motor)
