#include <stdio.h>
#include "pico/stdlib.h"
#include <time.h>

#define UART_ID uart0
#define BAUD_RATE 9600 // 9600 for bluetooth
#define UART_TX_PIN 0
#define UART_RX_PIN 1

clock_t clock()
{
    return (clock_t) time_us_64() / 10000;
}

int main(void) {
    stdio_init_all();

    // Initialise UART 0
    uart_init(UART_ID, BAUD_RATE);
 
    // Set the GPIO pin mux to the UART - 0 is TX, 1 is RX
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    char * testData = "1111111111";

    double executionTime = 0;
    double counter = 0;
    clock_t startTime = clock();
    
    // sending data over to HC05 using uart
    while (true) { 
        uart_puts(UART_ID, &testData);
        printf(testData);
        printf("\n");

        counter++;
        if (counter == 1000) { //10000 characters
            clock_t endTime = clock();
            executionTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
            // calculating time taken to send 100000 characters
            printf("Execution time:%f sec\n", executionTime);
            break;
        }
    }
}