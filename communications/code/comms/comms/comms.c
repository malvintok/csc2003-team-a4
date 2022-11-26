#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <math.h>
#include <time.h>

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

char temp[80];
char ch = 'c', coordX = 'c', coordY = 'y';

// ,ethods to send data over to M5Stick using UART
// each data will have an ID prepended to it
// # will be appended to each data to act as a delimiter
// left wheel speed ID = 1
void sendWheelSpeedL(int wheelspeed) {
    sprintf(temp, "1%d#",wheelspeed);
    uart_puts(UART_ID, temp);
    temp[80] = (char) 0;
}

// barcode data ID = 2
void sendBarcode(char * barcode) {
    sprintf(temp,"2%s#",barcode);
    uart_puts(UART_ID, temp);
    temp[80] = (char) 0;
}

// distance from sensor ID = 3
void sendDist(int dist) {
    sprintf(temp, "3%d#",dist);
    uart_puts(UART_ID, temp);
    temp[80] = (char) 0;
}

// height of hump ID = 4
void sendHeight(int height) {
    sprintf(temp, "4%d#",height);
    uart_puts(UART_ID, temp);   
    temp[80] = (char) 0;
}

// current position of car
// coord of car ID = 5
void sendCoord(int x, int y, int n, int s, int e, int w) {
    sprintf(temp, "5%d,%d,%d,%d,%d,%d#",x,y,n,s,e,w);
    uart_puts(UART_ID, temp);
    temp[80] = (char) 0;
}

// barcode coordinate ID = 6
void sendBarcodeCoord(int x, int y) {
    sprintf(temp, "6%d,%d#",x,y);
    uart_puts(UART_ID, temp);
    temp[80] = (char) 0;
}

// hump coordiaate ID = 7
void sendHumpCoord(int x, int y) {
    sprintf(temp, "7%d,%d#",x,y);
    uart_puts(UART_ID, temp);
    temp[80] = (char) 0;
}

// right wheel speed ID = 8
void sendWheelSpeedR(int wheelspeed) {
    sprintf(temp, "8%d#",wheelspeed);
    uart_puts(UART_ID, temp);
    temp[80] = (char) 0;
}

char getCoord() {
    char ch2 = uart_getc(UART_ID);
    return ch2;
}

int main() {
    stdio_init_all();

    // Set up our UART with the required speed.
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    char * barcode2 = "TEST";
    int wheelspeedl = 20;
    int wheelspeedr = 20;
    int distancefromsensor2 = 10;
    int heightofhump2 = 30;

    while(1) {
        // sending dummy data
        sendWheelSpeedL(wheelspeedl);
        wheelspeedl++;

        sendWheelSpeedR(wheelspeedr);
        wheelspeedr++;

        sendBarcode(barcode2);
        sendDist(distancefromsensor2);
        sendHeight(heightofhump2);

        sendCoord(0,1,0,0,0,0);
        sendCoord(0,2,0,0,0,0);
        sendCoord(0,3,0,0,0,0);
        sendCoord(-1,3,0,0,0,0);
        sendCoord(-2,3,0,0,0,0);
        sendCoord(-3,3,0,0,0,0);
        
        sendHumpCoord(-3,2);

        sendCoord(-3,1,0,0,0,0);
        sendCoord(-3,0,0,0,0,0);
        
        sendBarcodeCoord(-3,-1);

        sendCoord(-3,-2,0,0,0,0);
        sendCoord(-2,-2,0,0,0,0);
        sendCoord(-2,-3,0,0,0,0);
        sendCoord(-1,-3,0,0,0,0);
        sendCoord(0,-3,0,0,0,0);
        sendCoord(1,-3,0,0,0,0);
        sendCoord(1,-2,0,0,0,0);
        sendCoord(2,-2,0,0,0,0);

        // reading from m5
        // ch = getCoord();

        // once there is a comma, get the next ch and save it into y var
        // how to get previous char before comma to save as x?
        // create char array buffer to store every ch read
        // if comma found, get the [0] index from buffer, store next one into [1] index
        // else, replace the [0] index char array buffer
        // if (ch == ',') {
        //     coordY = getCoord();
        //     printf("Coord from flask: %c,%c", coordX, coordY);
        // }

        // send data every 1000ms
        sleep_ms(1000);
    }
}