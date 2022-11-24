#include <stdio.h>
#include "pico/stdlib.h"

#define UART_ID uart0
#define BAUD_RATE 9600 // 9600 for bluetooth
#define UART_TX_PIN 0
#define UART_RX_PIN 1

int main(void) {
    stdio_init_all();

     // Initialise UART 0
    uart_init(UART_ID, BAUD_RATE);
 
    // Set the GPIO pin mux to the UART - 0 is TX, 1 is RX
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // sending data over to HC05 using uart
    while (true) { 
        uart_puts(UART_ID, "Hello world!\n\r");
        sleep_ms(1000);
    }
}