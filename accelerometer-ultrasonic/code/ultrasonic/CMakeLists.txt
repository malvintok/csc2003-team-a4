add_executable(distance ultrasonic.c)

target_link_libraries(distance pico_stdlib hardware_uart hardware_gpio hardware_timer)

pico_enable_stdio_usb(distance 1)
pico_enable_stdio_uart(distance 0)

pico_add_extra_outputs(distance)

# example_auto_set_url(distance)