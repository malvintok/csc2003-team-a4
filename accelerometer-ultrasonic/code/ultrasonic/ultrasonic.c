#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define LeftTrig 10
#define LeftEcho 11
#define CenterTrig 14
#define CenterEcho 15
#define RightTrig 16
#define RightEcho 17
#define Interval 5000

const float cov[3] = {0.186389475, 0.000270548, 0.293938391};
typedef struct KalmanData
{
    float err_measurement; //Initialise variable
    float Q; //Process Noise Covariance (Taken from research paper)
    float KG; //Kalman Gain
    float E; //Estimated State
    float err_estimate; //Error in Estimate
}KalmanData;



static KalmanData kalmanData[3];
float val[10];

void init_pins(uint triggerPin, uint echoPin);
float getDist(int TriggerPin, int EchoPin);
float sma(float value);
float des(float value);
float kalman(float value, int i);


void init_pins(uint triggerPin, uint echoPin) {
    gpio_init(triggerPin);
    gpio_init(echoPin);
    gpio_set_dir(triggerPin, GPIO_OUT);
    gpio_set_dir(echoPin, GPIO_IN);
}

void init_kalmanStruct() {
    for(int i = 0; i < 3; ++i)
    {
        kalmanData[i].err_measurement = cov[i]; //Initialise variable
        kalmanData[i].Q = 0.01; //Noise Covariance Taken from research paper
        
        kalmanData[i].KG = 0; //Kalman Gain
        kalmanData[i].E = 0; //Estimated State
        kalmanData[i].err_estimate = cov[i]; //Error in Estimate
    }
}

float getDist(int TriggerPin, int EchoPin)
{
    gpio_put(TriggerPin, true);
    busy_wait_us(10); //  sleep on trigger high
    gpio_put(TriggerPin, false); // set trigger pin low
    gpio_put(EchoPin, false); // set trigger pin low

    // referred to https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf
    // wait for echo pin to go high (After sending 8 cycle burst, sensor will raise echo value to 1)
    while (gpio_get(EchoPin) == 0) {
        tight_loop_contents();
    }

    absolute_time_t startTime = get_absolute_time();
    // referred to https://lastminuteengineers.com/arduino-sr04-ultrasonic-sensor-tutorial/
    absolute_time_t timeout = delayed_by_ms(startTime, 38); //Obtain a time after 38ms (Echo Pin goes low after 38ms)
    while(gpio_get(EchoPin) == 1) {
        if(time_reached(timeout)) {
            return 400;
        }
        busy_wait_us(1);
    }
    absolute_time_t endTime = get_absolute_time();
    uint64_t diffTime = absolute_time_diff_us(startTime, endTime);

    // divide 2 to get the the original distance and multiply by 0.0343 (speed of sound) to get the distance in cm
    // 0.0343cm/us obtained from velocity of sound which is 343 m/s
    // 1 / 0.0343 = 29.1
    float distance = (diffTime / 2) / 29.1;
        
    return distance; //Return 
}


//SMA
float sma(float value) {
    static int count = 0;
    static int index = 0;
    float totalDist = 0;
    val[index] = value;

    for (int i = 0; i < 10; i++) {
        totalDist += val[i];
    }
    
    if (count != 10)
    {
        count += 1;
    }
    
    if (index == 9) {
        index = 0;
    }
    else {
        index += 1;
    }
    return (totalDist/count);
}

// Referred to https://www.youtube.com/watch?v=5KEy1OBnkJo&ab_channel=UnfoldDataScience
float des(float value) {
    static const float a = 0.8; //Weight of alpha
    static const float b = 0.8; //Weight of beta
    static float pSES = 0; //Previous DES reading
    static float cSES = 0; //Current DES reading
    static float cT = 0; //Trend estimation current
    static float pT = 0; //Trend estimation previous

    cSES = (a * value) + ((1-a) * (pSES + pT));
    cT = b*(cSES - pSES) + ((1-b) * (pT));
    pSES = cSES;
    pT = cT;
    return (cSES + cT);
}

// https://www.researchgate.net/publication/330540064_Kalman_Filter_Algorithm_Design_for_HC-SR04_Ultrasonic_Sensor_Data_Acquisition_System
// Q values referred from paper
float kalman(float value, int i) { //Value = Measured reading
    //1e-3: Able to reduce some noise and performance is faster
    //1e-5: Able to reduce noise better but have delay
    //1e-6: Strong noise reduction but severe delay

    //Update Kalman Gain
    kalmanData[i].KG = kalmanData[i].err_estimate / (kalmanData[i].err_estimate + kalmanData[i].err_measurement);
    //Estimating current state
    kalmanData[i].E = kalmanData[i].E + (kalmanData[i].KG*(value - kalmanData[i].E));
    //Predict and Update error in estimate
    kalmanData[i].err_estimate = ((1-kalmanData[i].KG)*kalmanData[i].err_estimate) + kalmanData[i].Q;
    //Return estimate value
    return kalmanData[i].E;
}


//Callback function to get distance
bool repeating_timer_callback(struct repeating_timer *t) {
    float distance = getDist(CenterTrig, CenterEcho);
    printf("Current Distance: %fcm\n\n", distance);
    return true;
}

int main()
{
    stdio_init_all();
    sleep_ms(5000);
    printf("Setting up pins...\n");
    init_kalmanStruct();
    init_pins(RightTrig, RightEcho);
    printf("Pins setup complete.\n");

    while(true) {
        float reading = getDist(RightTrig, RightEcho);
        printf("Raw Measurement: %fcm\n", reading);
        printf("SMA Measurement: %fcm\n", sma(reading));
        printf("DES Measurement: %fcm\n", des(reading));
        printf("KF  Measurement: %fcm\n\n", kalman(reading, 2));
        sleep_ms(500);
    }

    // Timer
    // struct repeating_timer timer;
    // add_repeating_timer_ms(Interval, repeating_timer_callback, NULL, &timer);
    // while (1)
    // {
    //     tight_loop_contents();
    // }
}