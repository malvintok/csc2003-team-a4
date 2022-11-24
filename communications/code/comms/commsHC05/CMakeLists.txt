add_executable(commsHC05
        commsHC05.c
        )

# pull in common dependencies
target_link_libraries(commsHC05 
        pico_stdlib
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(commsHC05)

# to see serial output
pico_enable_stdio_usb(commsHC05 1)
