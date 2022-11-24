add_executable(comms
        comms.c
        )

# pull in common dependencies
target_link_libraries(comms pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(comms)

# to see serial output
pico_enable_stdio_usb(comms 1)