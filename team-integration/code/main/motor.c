
#include "database.h"

// General definition(s)
#define WHEEL_TOTALNOTCH 20
#define WHEEL_CIRCUMFERENCE 8.5
#define MOTOR_SPEEDCAL_INTERVAL 500
#define MOTOR_PWMWRAP 3125

// Motor definition(s)
#define MOTOR_TURNINGPWM 1500
#define MOTOR_45DEGREETURN_NOTCH 5
#define MOTOR_90DEGREETURN_NOTCH 8
#define MOTOR_180DEGREETURN_NOTCH 15
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
// bool motor_actionSemaphore = true;
int motor_turnDegree = 0;
int  motor_leftNotchCount = 0, motor_rightNotchCount = 0;
int  motor_leftNetNotchCount = 0, motor_rightNetNotchCount = 0;
int  motor_leftSpeedNotchCount = 0, motor_rightSpeedNotchCount = 0;
float motor_leftSpeed = 0, motor_rightSpeed = 0;

float motor_leftAdjustPwm = 2000.0, motor_rightAdjustPwm = 2000.0;
float motor_leftPreviousErr = 0.0, motor_rightPreviousErr = 0.0;
float motor_leftSumErr = 0.0, motor_rightSumErr = 0.0;
float motor_leftError = 0.0, motor_rightError = 0.0;
float motor_leftPidSignal = 0.0, motor_rightPidSignal = 0.0;
float motor_leftAdjustPercent = 0.0, motor_rightAdjustPercent = 0.0;

int motor_targetTraverseNotch = 0;

struct repeating_timer timer_motor;

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
// - int degree: Turning degree [90 or 180]
void motors_turnMotorsNew(int direction, int degree) {
	// Set motor's action code to 2: turning
	motor_actionCode = 2;

	motor_turnDegree = degree;

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

	// Turn left
	if (direction == 1)
	{
		gpio_put(MOTOR_RIGHT_IN01_PIN, 1);
		gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
		gpio_put(MOTOR_LEFT_IN03_PIN, 0);
		gpio_put(MOTOR_LEFT_IN04_PIN, 1);

		gpio_put(MOTOR_RIGHT_ENABLE_PIN, 1);
		gpio_put(MOTOR_LEFT_ENABLE_PIN, 1);
	}
	// Turn right
	else
	{
		gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
		gpio_put(MOTOR_RIGHT_IN02_PIN, 1);
		gpio_put(MOTOR_LEFT_IN03_PIN, 1);
		gpio_put(MOTOR_LEFT_IN04_PIN, 0);

		gpio_put(MOTOR_RIGHT_ENABLE_PIN, 1);
		gpio_put(MOTOR_LEFT_ENABLE_PIN, 1);
	}
}

// Callback function of wheel encoders.
void motor_encoderCallback(uint gpio, uint32_t events)
{
	if (events == GPIO_IRQ_EDGE_FALL){
		if (gpio == INFARED_LEFT_ENCODER_PIN)
		{
			if (motor_actionCode == 1) {
				motor_leftNetNotchCount++;
			} else if (motor_actionCode == 2) {
				motor_leftNotchCount++;
			}
			motor_leftSpeedNotchCount++;
		}
		else if (gpio == INFARED_RIGHT_ENCODER_PIN)
		{
			if (motor_actionCode == 1) {
				motor_rightNetNotchCount++;
			} else if (motor_actionCode == 2) {
				motor_rightNotchCount++;
			}
			motor_rightSpeedNotchCount++;
		}
	}

	// Precision turning
	if (motor_actionCode == 2)
	{
		if (motor_turnDegree == 90) {
			if (motor_leftNotchCount >= MOTOR_90DEGREETURN_NOTCH)
			{
				gpio_put(MOTOR_LEFT_ENABLE_PIN, 0);
				gpio_put(MOTOR_LEFT_IN03_PIN, 0);
				gpio_put(MOTOR_LEFT_IN04_PIN, 0);
			}
			if (motor_rightNotchCount >= MOTOR_90DEGREETURN_NOTCH)
			{
				gpio_put(MOTOR_RIGHT_ENABLE_PIN, 0);
				gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
				gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
			}

			if (motor_rightNotchCount >= MOTOR_90DEGREETURN_NOTCH && motor_leftNotchCount >= MOTOR_90DEGREETURN_NOTCH)
			{
				motor_actionCode = 0;
				// motor_actionSemaphore = true;
				motor_leftNotchCount = 0;
				motor_rightNotchCount = 0;
				motors_driveNeutral();
			}
		} else if (motor_turnDegree == 180) {
			if (motor_leftNotchCount >= MOTOR_180DEGREETURN_NOTCH)
			{
				gpio_put(MOTOR_LEFT_ENABLE_PIN, 0);
				gpio_put(MOTOR_LEFT_IN03_PIN, 0);
				gpio_put(MOTOR_LEFT_IN04_PIN, 0);
			}
			if (motor_rightNotchCount >= MOTOR_180DEGREETURN_NOTCH)
			{
				gpio_put(MOTOR_RIGHT_ENABLE_PIN, 0);
				gpio_put(MOTOR_RIGHT_IN01_PIN, 0);
				gpio_put(MOTOR_RIGHT_IN02_PIN, 0);
			}

			if (motor_rightNotchCount >= MOTOR_180DEGREETURN_NOTCH && motor_leftNotchCount >= MOTOR_180DEGREETURN_NOTCH)
			{
				motor_actionCode = 0;
				// motor_actionSemaphore = true;
				motor_leftNotchCount = 0;
				motor_rightNotchCount = 0;
				motors_driveNeutral();
			}
		}
	}
	// Precision moving
	else if (motor_actionCode == 1) {
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
			// motor_actionSemaphore = true;
			motor_leftNetNotchCount = 0;
			motor_rightNetNotchCount = 0;
			// sleep_ms(50);
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
	database->wheelSpeedL = motor_leftSpeed = ((float)motor_leftSpeedNotchCount) * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);
	database->wheelSpeedR = motor_rightSpeed = ((float)motor_rightSpeedNotchCount) * ((float)WHEEL_CIRCUMFERENCE / (float)WHEEL_TOTALNOTCH);

	motor_leftSpeedNotchCount = 0;
	motor_rightSpeedNotchCount = 0;
	return true;
}

void motors_calculatePID() {
	//Avoid redundant processing
	if (motor_leftSpeed == 0 || motor_rightSpeed == 0){
		return;
	}
	
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

	//Calculate PID if both wheels' speeds are measured
	if (motor_leftSpeed > 0 && motor_rightSpeed > 0)
	{
		motor_leftAdjustPercent = 1.0 + (motor_leftPidSignal / motor_leftSpeed);
		motor_leftAdjustPwm = motor_leftAdjustPercent * motor_leftAdjustPwm;

		motor_rightAdjustPercent = 1.0 + (motor_rightPidSignal / motor_rightSpeed);
		motor_rightAdjustPwm = motor_rightAdjustPercent * motor_rightAdjustPwm;
	}
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
	motor_leftAdjustPwm = 2000.0, motor_rightAdjustPwm = 1700.0;
	motor_leftPreviousErr = 0.0, motor_rightPreviousErr = 0.0;
	motor_leftSumErr = 0.0, motor_rightSumErr = 0.0;
	motor_leftError = 0.0, motor_rightError = 0.0;
	motor_leftPidSignal = 0.0, motor_rightPidSignal = 0.0;
	motor_leftAdjustPercent = 0.0, motor_rightAdjustPercent = 0.0;

	// motor_actionSemaphore = false;

	motors_driveMotors(motor_leftAdjustPwm, motor_rightAdjustPwm);
}

void init_Motor()
{
	motor_initPins();
	motor_initPWM();
	
	add_repeating_timer_ms(MOTOR_SPEEDCAL_INTERVAL, motors_calculateSpeed, NULL, &timer_motor);
}
