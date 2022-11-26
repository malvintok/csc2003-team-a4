#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define CenterTrig 14
#define CenterEcho 15
#define LeftTrig 0
#define LeftEcho 0
#define RightTrig 0
#define RightEcho 0
#define Interval 5000
#define Cov1 0.000270548 // Sensor without cables
#define Cov2 0.000147731

const float cov[3] = {0.000270548, 0.000147731, 0};
typedef struct KalmanData
{
    float err_measurement; //Initialise variable
    float Q; //Noise Covariance Taken from research paper
    float KG; //Kalman Gain
    float E; //Estimated State
    float err_estimate; //Error in Estimate
}KalmanData;



static KalmanData kalmanData[3];
float val[10];

void init_pins(uint triggerPin, uint echoPin);
float getDist(int TriggerPin, int EchoPin);
uint firstReading();
float sma(float value);
float des(float value);
float kalman(float value, int i);


void init_pins(uint triggerPin, uint echoPin) {
    gpio_init(triggerPin);
    gpio_init(echoPin);
    gpio_set_dir(triggerPin, GPIO_OUT);
    gpio_set_dir(echoPin, GPIO_IN);

    for(int i = 0; i < 3; ++i)
    {
        kalmanData[i].err_measurement = cov[i]; //Initialise variable
        kalmanData[i].Q = 0.001; //Noise Covariance Taken from research paper
        
        kalmanData[i].KG = 0; //Kalman Gain
        kalmanData[i].E = 0; //Estimated State
        kalmanData[i].err_estimate = 0.001; //Error in Estimate
    }
}

float getDist(int TriggerPin, int EchoPin)
{
    static float measurement[5];
    static float temp = 0;
    //Take median of 5 readings
    for (int i=0; i<5; i++)
    {
        gpio_put(TriggerPin, true);
        busy_wait_us(10); // 10 microsecond sleep on trigger high
        gpio_put(TriggerPin, false); // set trigger pin low
        gpio_put(EchoPin, false); // set trigger pin low

        // wait for echo pin to go high
        while (gpio_get(EchoPin) == 0) {
            tight_loop_contents();
        }

        absolute_time_t startTime = get_absolute_time();
        while(gpio_get(EchoPin) == 1) {busy_wait_us(1);}//{sleep_us(1);}
        absolute_time_t endTime = get_absolute_time();
        uint64_t diffTime = absolute_time_diff_us(startTime, endTime);

        // divide 2 to get the the original distance and multiply by 0.0343 (speed of sound) to get the distance in cm
        // 0.0343cm/us obtained from velocity of sound which is 343 m/s
        // 1 / 0.0343 = 29.1
        float distance = (diffTime / 2) / 29.1;

        measurement[i] = distance;
        // busy_wait_ms(100);
        }

        //Sort the array in ascending order    
    for (int i = 0; i < 5; i++) {     
        for (int j = i+1; j < 5; j++) {     
           if(measurement[i] > measurement[j]) {    
               temp = measurement[i];    
               measurement[i] = measurement[j];    
               measurement[j] = temp;    
           }     
        }
    }
    int i;
    if  (TriggerPin == LeftTrig)
        i = 0;
    else if  (TriggerPin == CenterTrig)
        i = 1;
    else
        i = 2;

        
    return kalman(measurement[2], i); //Return median
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
// Not sure correct or not
float des(float value) {
    static const float a = 0.8; //Weight of alpha (Can change)
    static const float b = 0.8; //Weight of beta (Can change)
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

float kalman2(float value, float errM) { //Value = Measured reading
    //1e-3: Able to reduce some noise and performance is faster
    //1e-5: Able to reduce noise better but have delay
    //1e-6: Strong noise reduction but severe delay
    static float err_measurement = 0; //Initialise variable
    static const float Q = 0.001; //Noise Covariance Taken from research paper
    err_measurement = errM; //Measurement Covariance (Obtained from 100 readings)
    static float KG = 0; //Kalman Gain
    static float E = 0; //Estimated State
    static float err_estimate = 0.001; //Error in Estimate

    //Update Kalman Gain
    KG = err_estimate / (err_estimate + err_measurement);
    //Estimating current state
    E = E + (KG*(value - E));
    //Predict and Update error in estimate
    err_estimate = ((1-KG)*err_estimate) + Q;
    //Return estimate value
    return E;
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
    init_pins(CenterTrig, CenterEcho);
    printf("Pins setup complete.\n");

    for (int i = 0; i < 100; i++) {
        double distance = getDist(CenterTrig, CenterEcho);
        printf("%f\n", distance);
        sleep_ms(500);
    }

    // Timer?
    // struct repeating_timer timer;
    // add_repeating_timer_ms(Interval, repeating_timer_callback, NULL, &timer);
    // while (1)
    // {
    //     tight_loop_contents();
    // }
}