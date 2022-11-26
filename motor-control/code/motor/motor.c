#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "string.h"

// General definition(s)
#define WHEEL_TOTALNOTCH 20
#define WHEEL_CIRCUMFERENCE 8.5
#define MOTOR_SPEEDCAL_INTERVAL 500
#define MOTOR_PWMWRAP 3125

// Motor definition(s)
#define MOTOR_TURNINGPWM 1250
#define MOTOR_45DEGREETURN_NOTCH 5
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
bool motor_nextTurnSemaphore = true, motor_turning = false;
int  motor_leftNotchCount = 0, motor_rightNotchCount = 0;
int  motor_leftGridNotchCount = 0, motor_rightGridNotchCount = 0;
float motor_leftSpeed = 0, motor_rightSpeed = 0;

float motor_leftAdjustPwm = 2000.0, motor_rightAdjustPwm = 2000.0;
float motor_leftPreviousErr = 0.0, motor_rightPreviousErr = 0.0;
float motor_leftSumErr = 0.0, motor_rightSumErr = 0.0;
float motor_leftError = 0.0, motor_rightError = 0.0;
float motor_leftPidSignal = 0.0, motor_rightPidSignal = 0.0;
float motor_leftAdjustPercent = 0.0, motor_rightAdjustPercent = 0.0;

// Set motors to drive neutral
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
	motor_turning = true;

	// Calculate number of 45 degree turns required
	int numberOfTurns = degree / 45;

	// Set left motor PWM
	pwm_set_chan_level(
		pwm_gpio_to_slice_num(MOTOR_LEFT_ENABLE_PIN),
		pwm_gpio_to_channel(MOTOR_LEFT_ENABLE_PIN),
		MOTOR_TURNINGPWM);

	// Set right motor PWM
	pwm_set_chan_level(
		pwm_gpio_to_slice_num(MOTOR_RIGHT_ENABLE_PIN),
		pwm_gpio_to_channel(MOTOR_RIGHT_ENABLE_PIN),
		MOTOR_TURNINGPWM);

	for (int i = 0; i < numberOfTurns; i++)
	{
		motor_nextTurnSemaphore = false;

		// Turn left
		if (direction == 1)
		{
			gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
			gpio_put(MOTOR_RIGHT_IN02_PIN, 1);
			gpio_put(MOTOR_LEFT_IN03_PIN, 1);
			gpio_put(MOTOR_LEFT_IN04_PIN, 0);

			gpio_put(MOTOR_RIGHT_ENABLE_PIN, 1);
			gpio_put(MOTOR_LEFT_ENABLE_PIN, 1);
		}
		// Turn right
		else
		{
			gpio_put(MOTOR_RIGHT_IN01_PIN, 1);
			gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
			gpio_put(MOTOR_LEFT_IN03_PIN, 0);
			gpio_put(MOTOR_LEFT_IN04_PIN, 1);

			gpio_put(MOTOR_RIGHT_ENABLE_PIN, 1);
			gpio_put(MOTOR_LEFT_ENABLE_PIN, 1);
		}

		// Turn to desired degree
		while (!motor_nextTurnSemaphore)
		{
			sleep_ms(10);
		}
	}
	
	motor_turning = false;

	// Reset motor config
	motors_driveNeutral();
}

// Callback function of wheel encoders.
void motor_encoderCallback(uint gpio, uint32_t events)
{
	if (gpio == INFARED_LEFT_ENCODER_PIN)
	{
		if (events == GPIO_IRQ_EDGE_FALL)
		{
			motor_leftNotchCount++;
		}
	}
	else if (gpio == INFARED_RIGHT_ENCODER_PIN)
	{
		if (events == GPIO_IRQ_EDGE_FALL)
		{
			motor_rightNotchCount++;
		}
	}

	// For precision turning
	if (motor_turning)
	{
		if (motor_leftNotchCount >= MOTOR_45DEGREETURN_NOTCH)
		{
			gpio_put(MOTOR_LEFT_ENABLE_PIN, 0);
			gpio_put(MOTOR_LEFT_IN03_PIN, 0);
			gpio_put(MOTOR_LEFT_IN04_PIN, 0);
		}
		if (motor_rightNotchCount >= MOTOR_45DEGREETURN_NOTCH)
		{
			gpio_put(MOTOR_RIGHT_ENABLE_PIN, 0);
			gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
			gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
		}

		if (motor_rightNotchCount >= MOTOR_45DEGREETURN_NOTCH && motor_leftNotchCount >= MOTOR_45DEGREETURN_NOTCH)
		{
			motor_nextTurnSemaphore = true;
			motor_leftNotchCount = 0;
			motor_rightNotchCount = 0;
		}
	}
}

// Initialize pins for program runtimes
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
	motor_leftSpeed = motor_leftNotchCount * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);
	motor_rightSpeed = motor_rightNotchCount * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);

	if(!motor_turning) {
		motor_leftNotchCount = 0;
		motor_rightNotchCount = 0;
	}
	return true;
}

// Caculate PID for left and right wheel
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
}

int main()
{
	stdio_init_all();
	motor_initPins();
	motor_initPWM();

	struct repeating_timer timer;
	add_repeating_timer_ms(MOTOR_SPEEDCAL_INTERVAL, motors_calculateSpeed, NULL, &timer);

	while (1)
	{
		motors_calculatePID();
		motors_driveMotors(motor_leftAdjustPwm, motor_rightAdjustPwm);
	}
}