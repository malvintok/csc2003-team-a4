add_executable(accelerometer acc.c)

target_link_libraries(accelerometer pico_stdlib hardware_i2c)

pico_enable_stdio_usb(accelerometer 1)
pico_enable_stdio_uart(accelerometer 0)

pico_add_extra_outputs(accelerometer)

# example_auto_set_url(accelerometer)