#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "string.h"

#include "database.h"

// General definition(s)
#define WHEEL_TOTALNOTCH 20
#define WHEEL_CIRCUMFERENCE 8.5
#define MOTOR_SPEEDCAL_INTERVAL 500
#define MOTOR_PWMWRAP 3125

// Motor definition(s)
#define MOTOR_TURNINGPWM 1500
#define MOTOR_DEGREETURN_NOTCH 5
#define MOTOR_RIGHT_IN01_PIN 6
#define MOTOR_RIGHT_IN02_PIN 7
#define MOTOR_RIGHT_ENABLE_PIN 8
#define MOTOR_LEFT_IN03_PIN 4
#define MOTOR_LEFT_IN04_PIN 3
#define MOTOR_LEFT_ENABLE_PIN 2

// Motor PID definition(s)
#define MOTOR_LEFT_SETSPEED 5.0f
#define MOTOR_RIGHT_SETSPEED 5.0f
#define MOTOR_LEFT_KP 1.0F
#define MOTOR_RIGHT_KP 1.0F
#define MOTOR_LEFT_KI 0.0F
#define MOTOR_RIGHT_KI 0.0F
#define MOTOR_LEFT_KD 0.0F
#define MOTOR_RIGHT_KD 0.0F

// Infared definition(s)
#define INFARED_LEFT_ENCODER_PIN 21
#define INFARED_RIGHT_ENCODER_PIN 22

// Global variable(s)
bool motor_actionSemaphore = true; // 0: Idle, 1: GoingForward, 2: Turning
int  motor_leftNotchCount = 0, motor_rightNotchCount = 0;
int  motor_leftNetNotchCount = 0, motor_rightNetNotchCount = 0;
float motor_leftSpeed = 0, motor_rightSpeed = 0;

float motor_leftAdjustPwm = 2000.0, motor_rightAdjustPwm = 2000.0;
float motor_leftPreviousErr = 0.0, motor_rightPreviousErr = 0.0;
float motor_leftSumErr = 0.0, motor_rightSumErr = 0.0;
float motor_leftError = 0.0, motor_rightError = 0.0;
float motor_leftPidSignal = 0.0, motor_rightPidSignal = 0.0;
float motor_leftAdjustPercent = 0.0, motor_rightAdjustPercent = 0.0;

int motor_targetTraverseNotch = 0;

struct repeating_timer motor_timer;

// Drive neutral function of motors.
void motors_driveNeutral()
{
	gpio_put(MOTOR_RIGHT_ENABLE_PIN, 0);
	gpio_put(MOTOR_LEFT_ENABLE_PIN, 0);

	gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
	gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
	gpio_put(MOTOR_LEFT_IN03_PIN, 0);
	gpio_put(MOTOR_LEFT_IN04_PIN, 0);
}

// Drive forward function of motors.
// Parameters:
// - int leftPwm: PWM of left wheel
// - int rightPwm: PWM of right wheel
void motors_driveMotors(float leftPwm, float rightPwm)
{
	// printf("Motor.c Driving Motors Forward...\n");
	// Set left motor PWM
	pwm_set_chan_level(
		pwm_gpio_to_slice_num(MOTOR_LEFT_ENABLE_PIN),
		pwm_gpio_to_channel(MOTOR_LEFT_ENABLE_PIN),
		leftPwm);

	// Set right motor PWM
	pwm_set_chan_level(
		pwm_gpio_to_slice_num(MOTOR_RIGHT_ENABLE_PIN),
		pwm_gpio_to_channel(MOTOR_RIGHT_ENABLE_PIN),
		rightPwm);

	gpio_put(MOTOR_RIGHT_IN01_PIN, 1);
	gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
	gpio_put(MOTOR_LEFT_IN03_PIN, 1);
	gpio_put(MOTOR_LEFT_IN04_PIN, 0);

	gpio_put(MOTOR_RIGHT_ENABLE_PIN, 1);
	gpio_put(MOTOR_LEFT_ENABLE_PIN, 1);
}

// Turn function of motors.
// Parameters:
// - int direction: Turning direction [0: Right, 1: Left]
// - int degree: Turning degree [Min: 45, Max: 45 * n]
void motors_turnMotors(int direction, int degree)
{
	//printf("Motor.c Turning Motors...\n");
	// Set motor's action code to 2: turning
	motor_actionCode = 2;

	// Calculate number of 45 degree turns required
	int numberOfTurns = degree / 45;

	//printf("Checkpoint01\n");

	// Set left motor PWM
	pwm_set_chan_level(
		pwm_gpio_to_slice_num(MOTOR_LEFT_ENABLE_PIN),
		pwm_gpio_to_channel(MOTOR_LEFT_ENABLE_PIN),
		MOTOR_TURNINGPWM);

	//printf("Checkpoint02\n");

	// Set right motor PWM
	pwm_set_chan_level(
		pwm_gpio_to_slice_num(MOTOR_RIGHT_ENABLE_PIN),
		pwm_gpio_to_channel(MOTOR_RIGHT_ENABLE_PIN),
		MOTOR_TURNINGPWM);

	//printf("Checkpoint03\n");

	// Turn left
	if (direction == 1)
	{
		gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
		gpio_put(MOTOR_RIGHT_IN02_PIN, 1);
		gpio_put(MOTOR_LEFT_IN03_PIN, 1);
		gpio_put(MOTOR_LEFT_IN04_PIN, 0);
	}
	// Turn right
	else
	{
		gpio_put(MOTOR_RIGHT_IN01_PIN, 1);
		gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
		gpio_put(MOTOR_LEFT_IN03_PIN, 0);
		gpio_put(MOTOR_LEFT_IN04_PIN, 1);
	}

	//printf("Checkpoint04\n");

	for (int i = 0; i < numberOfTurns; i++)
	{
		//printf("Checkpoint05 %d\n", i);

		motor_actionSemaphore = false;

		//printf("Checkpoint06 %d\n", i);

		gpio_put(MOTOR_RIGHT_ENABLE_PIN, 1);
		gpio_put(MOTOR_LEFT_ENABLE_PIN, 1);

		//printf("Checkpoint07 %d\n", i);

		// Turn to desired degree
		while (!motor_actionSemaphore)
		{
			//printf("Sleeping\n");

			//sleep_ms(10);
		}

		//printf("Checkpoint08 %d\n", i);
	}
	
	//printf("Checkpoint09\n");

	// Set motor's action code to 0: idle
	motor_actionCode = 0;

	//printf("Checkpoint10\n");

	// Reset motor config
	motors_driveNeutral();
	//printf("I'm done turning, bye bye :)\n");
}

// Callback function of wheel encoders.
void motor_encoderCallback(uint gpio, uint32_t events)
{
	if (events == GPIO_IRQ_EDGE_FALL){
		if (gpio == INFARED_LEFT_ENCODER_PIN)
		{
			motor_leftNotchCount++;
			motor_leftNetNotchCount++;
		}
		else if (gpio == INFARED_RIGHT_ENCODER_PIN)
		{
			motor_rightNotchCount++;
			motor_rightNetNotchCount++;
		}
	}

	// Precision turning
	if (motor_actionCode == 2)
	{
		//printf("Interrupt Checkpoint01\n");

		// Check if left wheel turned 5 notches
		if (motor_leftNotchCount >= MOTOR_DEGREETURN_NOTCH)
		{
			//printf("Interrupt Checkpoint0111\n");
			gpio_put(MOTOR_LEFT_ENABLE_PIN, 0);
			gpio_put(MOTOR_LEFT_IN03_PIN, 0);
			gpio_put(MOTOR_LEFT_IN04_PIN, 0);
		}

		//printf("Interrupt Checkpoint0111\n");
		
		// Check if right wheel turned 5 notches
		if (motor_rightNotchCount >= MOTOR_DEGREETURN_NOTCH)
		{
			gpio_put(MOTOR_RIGHT_ENABLE_PIN, 0);
			gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
			gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
		}

		//printf("Interrupt Checkpoint02\n");

		// Check if both wheels turned 5 notches
		if (motor_rightNotchCount >= MOTOR_DEGREETURN_NOTCH && motor_leftNotchCount >= MOTOR_DEGREETURN_NOTCH)
		{
			//printf("Interrupt Checkpoint03\n");
			
			motor_actionCode = 0;
			motor_actionSemaphore = true;
			motor_leftNotchCount = 0;
			motor_rightNotchCount = 0;
			motors_driveNeutral();

			sleep_ms(50);
		}
	}
	// Precision moving
	else if(motor_actionCode == 1){
		// Check if left wheel reached target notch count to traverse
		if (motor_leftNetNotchCount >= motor_targetTraverseNotch){
			gpio_put(MOTOR_LEFT_ENABLE_PIN, 0);
			gpio_put(MOTOR_LEFT_IN03_PIN, 0);
			gpio_put(MOTOR_LEFT_IN04_PIN, 0);
		}
		// Check if right wheel reached target notch count to traverse
		if (motor_rightNetNotchCount >= motor_targetTraverseNotch){
			gpio_put(MOTOR_RIGHT_ENABLE_PIN, 0);
			gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
			gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
		}

		// Check if both wheels reached target notch count to traverse
		if (motor_rightNetNotchCount >= motor_targetTraverseNotch && 
		motor_leftNetNotchCount >= motor_targetTraverseNotch)
		{
			motor_actionCode = 0;
			motor_actionSemaphore = true;
			motor_leftNetNotchCount = 0;
			motor_rightNetNotchCount = 0;
			motors_driveNeutral();

			//sleep_ms(100);
		}
	}
}

// Initialize pins for motor module components.
void motor_initPins()
{
	// Right Wheel pin initailization(s)
	gpio_init(MOTOR_RIGHT_IN01_PIN);
	gpio_init(MOTOR_RIGHT_IN02_PIN);
	gpio_init(MOTOR_RIGHT_ENABLE_PIN);

	// Right Wheel pin direction initailization(s)
	gpio_set_dir(MOTOR_RIGHT_IN01_PIN, GPIO_OUT);
	gpio_set_dir(MOTOR_RIGHT_IN02_PIN, GPIO_OUT);
	gpio_set_dir(MOTOR_RIGHT_ENABLE_PIN, GPIO_OUT);

	// Left Wheel pin initailization(s)
	gpio_init(MOTOR_LEFT_IN03_PIN);
	gpio_init(MOTOR_LEFT_IN04_PIN);
	gpio_init(MOTOR_LEFT_ENABLE_PIN);

	// Left Wheel pin direction initailization(s)
	gpio_set_dir(MOTOR_LEFT_IN03_PIN, GPIO_OUT);
	gpio_set_dir(MOTOR_LEFT_IN04_PIN, GPIO_OUT);
	gpio_set_dir(MOTOR_RIGHT_ENABLE_PIN, GPIO_OUT);

	// Encoder pin initialization(s)
	gpio_init(INFARED_LEFT_ENCODER_PIN);
	gpio_init(INFARED_RIGHT_ENCODER_PIN);

	// Encoder pin direction initailization(s)
	gpio_set_dir(INFARED_LEFT_ENCODER_PIN, GPIO_IN);
	gpio_set_dir(INFARED_RIGHT_ENCODER_PIN, GPIO_IN);
	gpio_pull_up(INFARED_LEFT_ENCODER_PIN);
	gpio_pull_up(INFARED_RIGHT_ENCODER_PIN);

	gpio_set_irq_enabled_with_callback(INFARED_LEFT_ENCODER_PIN, GPIO_IRQ_EDGE_FALL, true, &motor_encoderCallback);
	gpio_set_irq_enabled(INFARED_RIGHT_ENCODER_PIN, GPIO_IRQ_EDGE_FALL, true);
}

// Initialize PWM for motor.
void motor_initPWM() {
	gpio_set_function(MOTOR_LEFT_ENABLE_PIN, GPIO_FUNC_PWM);
	gpio_set_function(MOTOR_RIGHT_ENABLE_PIN, GPIO_FUNC_PWM);
	uint slice_left = pwm_gpio_to_slice_num(MOTOR_LEFT_ENABLE_PIN);
	uint channel_left = pwm_gpio_to_channel(MOTOR_LEFT_ENABLE_PIN);
	uint slice_right = pwm_gpio_to_slice_num(MOTOR_RIGHT_ENABLE_PIN);
	uint channel_right = pwm_gpio_to_channel(MOTOR_RIGHT_ENABLE_PIN);

	pwm_set_wrap(slice_left, MOTOR_PWMWRAP);  // Frequency 40kHz, Clock Freq 125MHz
	pwm_set_phase_correct(slice_left, false); // Count Up Mode
	pwm_set_clkdiv(slice_left, 125);

	pwm_set_wrap(slice_right, MOTOR_PWMWRAP);  // Frequency 40kHz, Clock Freq 125MHz
	pwm_set_phase_correct(slice_right, false); // Count Up Mode
	pwm_set_clkdiv(slice_right, 125);

	pwm_set_enabled(slice_left, true);	// Start Generating Signal
	pwm_set_enabled(slice_right, true); // Start Generating Signal
}

// Calculate speed of motors.
bool motors_calculateSpeed(struct repeating_timer *t)
{
	//database->wheelSpeedL = motor_leftSpeed = motor_leftNotchCount * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);
	//database->wheelSpeedR = motor_rightSpeed = motor_rightNotchCount * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);

	motor_leftSpeed = motor_leftNotchCount * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);
	motor_rightSpeed = motor_rightNotchCount * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);
	if(motor_actionCode == 1) {
		motor_leftNotchCount = 0;
		motor_rightNotchCount = 0;
	}
	return true;
}

void motors_calculatePID() {
	// Left Wheel PID
	motor_leftError = MOTOR_LEFT_SETSPEED - motor_leftSpeed;
	motor_leftPidSignal = 
		(motor_leftError * MOTOR_LEFT_KP) + 
		(motor_leftSumErr * MOTOR_LEFT_KI) + 
		((motor_leftError - motor_leftPreviousErr) * MOTOR_LEFT_KD);
	motor_leftPreviousErr = motor_leftError;
	motor_leftSumErr += motor_leftError;
	if (motor_leftSumErr > MOTOR_LEFT_SETSPEED)
		motor_leftSumErr = MOTOR_LEFT_SETSPEED;
	if (motor_leftSumErr < (MOTOR_LEFT_SETSPEED / 2))
		motor_leftSumErr = MOTOR_LEFT_SETSPEED / 2;

	// Right Wheel PID
	motor_rightError = MOTOR_RIGHT_SETSPEED - motor_rightSpeed;
	motor_rightPidSignal = 
		(motor_rightError * MOTOR_RIGHT_KP) + 
		(motor_rightSumErr * MOTOR_RIGHT_KI) + 
		((motor_rightError - motor_rightPreviousErr) * MOTOR_RIGHT_KD);
	motor_rightPreviousErr = motor_rightError;
	motor_rightSumErr += motor_rightError;
	if (motor_rightSumErr > MOTOR_RIGHT_SETSPEED)
		motor_rightSumErr = MOTOR_RIGHT_SETSPEED;
	if (motor_rightSumErr < (MOTOR_RIGHT_SETSPEED / 2))
		motor_rightSumErr = MOTOR_RIGHT_SETSPEED / 2;


	if (motor_leftSpeed > 0 && motor_rightSpeed > 0)
	{
		motor_leftAdjustPercent = 1.0 + (motor_leftPidSignal / motor_leftSpeed);
		motor_rightAdjustPercent = 1.0 + (motor_rightPidSignal / motor_rightSpeed);

		motor_leftAdjustPwm = motor_leftAdjustPercent * motor_leftAdjustPwm;
		motor_rightAdjustPwm = motor_rightAdjustPercent * motor_rightAdjustPwm;

		motor_leftSpeed = 0, motor_rightSpeed = 0;
	}

	motors_driveMotors(motor_leftAdjustPwm, motor_rightAdjustPwm);
}

void motors_moveByNotch(int numOfNotches) {
	if(numOfNotches > 20){
		numOfNotches -= 6 + (numOfNotches / 20);
	}

	motor_targetTraverseNotch = numOfNotches;
	motor_leftNetNotchCount = 0, motor_rightNetNotchCount = 0;

	// Set motor's action code to 1: GoingForward
	motor_actionCode = 1;

	//Reset all PID variables
	motor_leftAdjustPwm = 2000.0, motor_rightAdjustPwm = 2000.0;
	motor_leftPreviousErr = 0.0, motor_rightPreviousErr = 0.0;
	motor_leftSumErr = 0.0, motor_rightSumErr = 0.0;
	motor_leftError = 0.0, motor_rightError = 0.0;
	motor_leftPidSignal = 0.0, motor_rightPidSignal = 0.0;
	motor_leftAdjustPercent = 0.0, motor_rightAdjustPercent = 0.0;

	motor_actionSemaphore = false;

	motors_driveMotors(motor_leftAdjustPwm, motor_rightAdjustPwm);
}

void init_Motor()
{
	motor_initPins();
	motor_initPWM();
	
	add_repeating_timer_ms(MOTOR_SPEEDCAL_INTERVAL, motors_calculateSpeed, NULL, &motor_timer);
}

int maina()
{
	stdio_init_all();
	motor_initPins();
	motor_initPWM();

	add_repeating_timer_ms(MOTOR_SPEEDCAL_INTERVAL, motors_calculateSpeed, NULL, &motor_timer);
	
	// sleep_ms(2000);
	// printf("==========================================");

	// motors_moveByNotch(100);

	// // Go straight for 100 notches
	// while (!motor_actionSemaphore)
	// {
	// 	// If motor's action code is 1: GoingForward
	// 	if(motor_actionCode == 1){
	// 		motors_calculatePID();
	// 	}
	// }

	// sleep_ms(500);

	// Turn 45 right
	while (1) {
		motors_turnMotors(0, 90);
		sleep_ms(2000);
	}
	

}