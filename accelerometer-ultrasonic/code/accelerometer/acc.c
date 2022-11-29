#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vector.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

/* 
   Documentation Link: https://core-electronics.com.au/attachments/localcontent/MPU-6050_DataSheet_V34_14872bbfb20.pdf
*/

// By default these devices  are on bus address 0x68
static int addr = 0x68; // MPU6050 I2C address

#define MOVINGAVGSIZE 10 // sets size of moving average array
#define AVERAGEMOTORSPEED 5 // obtained from comms team, if integrate, can pull value in real time

// used by comms team
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

char temp[80]; 

// function called to send height to comms team
// height of hump ID = 4
void sendHeight(float height) {
    sprintf(temp, "4%.2f#",height);
    uart_puts(UART_ID, temp);   
    temp[80] = (char) 0;
}

#ifdef i2c_default
static void mpu6050_reset() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x6B, 0x00};
    i2c_write_blocking(i2c_default, addr, buf, 2, false);
}

static void mpu6050_read_raw(accel_vector *data) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);

    data->accX = (buffer[0] << 8 | buffer[1]);
    data->accY = (buffer[2] << 8 | buffer[3]);
    data->accZ = (buffer[4] << 8 | buffer[5]);
}

// moving average filter
// credits https://gist.github.com/bmccormack/d12f4bf0c96423d03f82
float movingAverageHelper(float *ptrArrNumbers, float *ptrSum, int *ptrIndex, float nextNum) {
    *ptrSum = *ptrSum - ptrArrNumbers[*ptrIndex] + nextNum; // increment sum by nextNum, decrement by oldest number
    ptrArrNumbers[*ptrIndex] = nextNum; // adds the new number to the array
    *ptrIndex = (*ptrIndex + 1) % MOVINGAVGSIZE; // increment index, reset the index to 0 when it reaches the end of the array
    return *ptrSum / MOVINGAVGSIZE;
}


static void moving_average(accel_vector *data) {
    // reads the raw data and applies a moving average filter
    // then prints both the raw data and the filtered data
    // to console
    float xArray[MOVINGAVGSIZE] = {0}; // used to keep moving average acc x values
    float yArray[MOVINGAVGSIZE] = {0}; // used to keep moving average acc y values
    float sumX = 0; // used to keep track of the sum of the moving average acc x values
    float sumY = 0; // used to keep track of the sum of the moving average acc y values
    float maX = 0; // used to keep track of the moving average acc x value
    float maY = 0; // used to keep track of the moving average acc y value
    int indexX = 0; // used to keep track of the index of the moving average acc x array
    int indexY = 0; // used to keep track of the index of the moving average acc y array

    while (1)
    {
        mpu6050_read_raw(data);

        float maX = movingAverageHelper(xArray, &sumX, &indexX, data->accX);
        float maY = movingAverageHelper(yArray, &sumY, &indexY, data->accY);
        printf("original x: %f, original y: %f\n", data->accX, data->accY);
        printf("maX : %f, maY : %f\n", maX, maY);
        //float roll = atan2(averageY, averageZ) * 180 / M_PI; // https://wiki.dfrobot.com/How_to_Use_a_Three-Axis_Accelerometer_for_Tilt_Sensing
        sleep_ms(1000);
    }
}

// kalman filter
// credits https://www.researchgate.net/publication/342852022_Kalman_Filter_for_Noise_Reducer_on_Sensor_Readings
float kalmanHelper(float R, float Q, float measuredX, float *Xt_prev, float *Pt_prev)
{
    // 𝑄 is process variance matrix
    // 𝑅 is measurement matrix
    float Xt_update = *Xt_prev;
    float Pt_update = *Pt_prev + Q;
    float Kt = Pt_update / (Pt_update + R);
    float Xt = Xt_update + (Kt * (measuredX - Xt_update));
    float Pt = (1 - Kt) * Pt_update;
    *Xt_prev = Xt;
    *Pt_prev = Pt;
    return Xt;
}

float filterData() {
    float rawX, movingAvgX, kalmanX;
    accel_vector data;
    // moving average variables
    float static xAngleArray[MOVINGAVGSIZE] = {0};
    float static xSum = 0; // store moving average sum
    int static xIndex = 0; // store moving average index
    // kalman variables
    float static Xt_prev; 
    float static Pt_prev = 1.0; // initial variance
    
    mpu6050_read_raw(&data);
    // calculate tilt angle using formula provided
    // source: https://wiki.dfrobot.com/How_to_Use_a_Three-Axis_Accelerometer_for_Tilt_Sensing
    rawX = atan2(data.accY, data.accZ) * 180 / M_PI; 
    movingAvgX = movingAverageHelper(xAngleArray, &xSum, &xIndex, rawX);
    kalmanX = kalmanHelper(0.1, 0.1, rawX, &Xt_prev, &Pt_prev);
    printf("rawX: %.2f, movingAvgX: %.2f, Kalman: %.2f\n", rawX, movingAvgX, kalmanX);

    return kalmanX;
}

void checkHump(float kalmanX) {
    float initialError = 5; // sets value to ignore from
    bool static hump = false;
    absolute_time_t static startHumpTime;
    absolute_time_t static endHumpTime;
    float static highestHeight = 0;

    if (kalmanX > initialError) {
        // encountered a hump and get the current time
        if (!hump) {
            hump = !hump; // set hump to detected
            startHumpTime = get_absolute_time();
            highestHeight = kalmanX;
        }
        else {
            // updates the highest height
            if (kalmanX > highestHeight) {
                endHumpTime = get_absolute_time();
                highestHeight = kalmanX;
            }
        }
    }
    else if (kalmanX < highestHeight) {
        // encountered a valley and get the current time
        if (hump) {
            hump = !hump; // reset hump status
            float timeTook = absolute_time_diff_us(startHumpTime, endHumpTime) / 1000000.0; //get seconds of time taken
            float distance = timeTook * AVERAGEMOTORSPEED; // distance = speed * time
            // using sin(theta) = opp/hypo
            // to get height:
            //opp = sin(theta in rad) * hypo
            float height = sin(highestHeight * M_PI / 180) * distance;
            if (height > 0) {
                printf("Height of Hump: %.2f\n", height, highestHeight, timeTook);
                sendHeight(height); // for integration to send to comms
            }
            // reset variables
            highestHeight = 0.0;
        }
    }
}
#endif


int main() {
    stdio_init_all();
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/mpu6050_i2c example requires a board with I2C pins
    puts("Default I2C pins were not defined");
#else
    printf("Hello, MPU6050! Reading raw data from registers...\n");

    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
        // Set up our UART with the required speed.
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    mpu6050_reset();
    float kalmanX;

    while (1) {
        // read and apply kalman filter, then check for hump
        // if there's a hump, print height and send to comms
        kalmanX = filterData();
        checkHump(kalmanX);
        sleep_ms(200);
    }

#endif
    return 0;
}