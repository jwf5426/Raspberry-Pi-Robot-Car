/**************************************************
* CMPEN 473, Spring 2018, Penn State University
* 
* Homework 7
* Revision V1.0
* On 4/24/17
* By James Frazier
* 
***************************************************/

/* Homework 7
 * Program allows to control montion of car with keystrokes
 * You can change the pins, keys, steps for speed, and steps for steering 
 * below where all the defines are.
 * Use IMU to collect accleration, rotation, and magnetic field
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>
#include "import_registers.h"
#include "gpio.h"
#include "pwm.h"
#include "spi.h"
#include <wiringPi.h>
#include <softPwm.h>
#include <pthread.h>
#include "math.h"
#include "accelerometer_project.h"
#include "hw7frazier_camera.h"

#define PWMA 12
#define PWMB 13
#define AIN1 5
#define AIN2 6
#define BIN1 22
#define BIN2 23
#define IR_LEFT 24
#define IR_CENTER 25
#define IR_RIGHT 26
#define left_motor_calibration -9 // for a range of 200
#define right_motor_calibration 0 // for a range of 200
#define steering_steps 5
#define speed_steps 10
#define pwm_range 200
#define manual 'm'
#define accelerometer_max_x 0
#define accelerometer_min_x 0
#define accelerometer_max_y 0
#define accelerometer_min_y 0
#define gyroscope_max_x 0
#define gyroscope_min_x 0
#define gyroscope_max_y 0
#define gyroscope_min_y 0
#define magnetometer_max_x 0
#define magnetometer_min_x 0
#define magnetometer_max_y 0
#define magnetometer_min_y 0


#define stop 's'
#define forward 'f'
#define backward 'b'
#define faster 'i'
#define slower 'd'
#define left 'l'
#define right 'r'


/* Uncomment for WASD, and comment above
#define stop 'p' // 's'
#define forward 'i' // 'f'
#define backward 'o' // 'b'
#define faster 'w' // 'i'
#define slower 's' // 'd'
#define left 'a' // 'l'
#define right 'd' // 'r'
*/

typedef struct XYZs {
	struct XYZ* xyz;
	unsigned long int* time;
	int count;
} XYZs;

typedef struct MaxMin {
	float max_x;
	float min_x;
	float max_y;
	float min_y;
} MaxMin;

char gear = 's';
int speed_precent = 0;
int left_precent = 0;
int right_precent = 0;
int left_pwm = 0;
int right_pwm = 0;
int CAMERA_STOP_THREAD = 0;
int m_one_accelerometer_stop_thread = 0;
int m_two_stop_thread = 0;
int manual_state = 1;
XYZs accelerometer;
XYZs gyroscope;
XYZs magnetometer;
MaxMin accel;
MaxMin gyro;
MaxMin mag;
struct XYZ accel_result;
struct XYZ gyro_result;
struct XYZ mag_result;

int get_pressed_key(void)
{
	int ch;

	// refer to man page for stty
	system("stty -icanon -echo"); // sets min number of characters for a read to one
	ch = getchar();
	system("stty icanon echo"); // restores previous terminal settings

	return ch;
}

int updatePwm() { // Update the left and right pwm for changing speed and direction
	int set_left_pwm = left_pwm * (1 - ((double) left_precent / 100)) + left_motor_calibration;
	if(set_left_pwm < 0) {
		softPwmWrite(PWMA, 0);
	} else {
		softPwmWrite(PWMA, set_left_pwm);
	}
	
	int set_right_pwm = right_pwm * (1 - ((double) right_precent / 100)) + right_motor_calibration;
	if(set_right_pwm < 0) {
		softPwmWrite(PWMB, 0);
	} else {
		softPwmWrite(PWMB, set_right_pwm);
	}

	return 0;
}

int accelerate() { // Step up the speed by one step
	if(speed_precent < 100 && speed_precent + (100 / speed_steps) <= 100) {
		left_pwm += pwm_range / speed_steps;
		right_pwm += pwm_range / speed_steps;
		
		updatePwm();
		
		speed_precent += 100 / speed_steps;
	}
	
	return speed_precent;
}

int decelerate() { // Step down the speed by one step
	if(speed_precent > 0 && speed_precent - (100 / speed_steps) >= 0) {
		left_pwm -= pwm_range / speed_steps;
		right_pwm -= pwm_range / speed_steps;
		
		updatePwm();
		
		speed_precent -= 100 / speed_steps;
	}
	
	return speed_precent;
}

int steerLeft() { // Steer to left by one step
	if(right_precent > 0 && right_precent - (100 / steering_steps) >= 0) {
		right_precent -= 100 / steering_steps;
	}
	else if(left_precent < 100 && left_precent + (100 / steering_steps) <= 100) {
		left_precent += 100 / steering_steps;
	}
	
	updatePwm();
	
	return left_precent;
}

int steerRight() { // Steer to right by one step
	if(left_precent > 0 && left_precent - (100 / steering_steps) >= 0) {
		left_precent -= 100 / steering_steps;
	}
	else if(right_precent < 100 && right_precent + (100 / steering_steps) <= 100) {
		right_precent += 100 / steering_steps;
	}
	
	updatePwm();
	
	return right_precent;
}

int steerStraight() { // Set left and right steer present to 0
	left_precent = 0;
	right_precent = 0;
	
	updatePwm();
	
	return 0;
}

char park() { // Stop
	while(speed_precent > 0) { // Gradually slow down
		decelerate();
		usleep(100000); // wait 0.1 ms
	}
	
	GPIO_CLR( &(io->gpio), AIN1);
	GPIO_CLR( &(io->gpio), AIN2);
	GPIO_CLR( &(io->gpio), BIN1);
	GPIO_CLR( &(io->gpio), BIN2);
	
	gear = 's';
	return gear;
}

char drive() { // Forward
	int temp_speed_precent = speed_precent;
	while(speed_precent > 0) { // Gradually slow down
		decelerate();
		usleep(100000); // wait 0.1 ms
	}
	
	GPIO_CLR( &(io->gpio), AIN1);
	GPIO_SET( &(io->gpio), AIN2);
	GPIO_CLR( &(io->gpio), BIN1);
	GPIO_SET( &(io->gpio), BIN2);
				
	usleep(500000);	// wait 0.1 ms
	while(speed_precent < temp_speed_precent) { // Gradually increase speed
		accelerate();
		usleep(100000);
	}
				
	gear = 'f';
	return gear;
}

char reverse() {
	int temp_speed_precent = speed_precent;
	while(speed_precent > 0) { // Gradually decrease speed
		decelerate();
		usleep(100000); // wait 0.1 ms
	}
	
	GPIO_SET( &(io->gpio), AIN1);
	GPIO_CLR( &(io->gpio), AIN2);
	GPIO_SET( &(io->gpio), BIN1);
	GPIO_CLR( &(io->gpio), BIN2);
	
	usleep(500000);	// wait 0.1 ms
	while(speed_precent < temp_speed_precent) { // Gradually increase speed
		accelerate();
		usleep(100000);
	}
	
	gear = 'b';
	return gear;
}

int getZeroToNine(float reading, float min, float max) {
	float steps = (max - min) / 10;
	for(int i = 0; i < 10; i++) {
		if(reading >= min + steps * i && reading <= min + steps * (i + 1)) {
			return i;
		}
	}
	return -1;
}

void updateTerminal() { // Terminal thread
	printf("%d%d%d%d%d%d\n", getZeroToNine(accel_result.x, accel.min_x, accel.max_x), getZeroToNine(accel_result.y, accel.min_y, accel.max_y), getZeroToNine(gyro_result.x, gyro.min_x, gyro.max_x), getZeroToNine(gyro_result.y, gyro.min_y, gyro.max_y), getZeroToNine(mag_result.x, mag.min_x, mag.max_x), getZeroToNine(mag_result.y, mag.min_y, mag.max_y));

	/*if(gear == 's') {
		printf("[P] R D M%d Speed[%03d%%] Steering_Left[%03d%%] Steering_Right[%03d%%] M2 Only: Accel[%d][%d] Gyro[%d][%d] Mag[%d][%d]\r", manual_state, speed_precent, left_precent, right_precent, getZeroToNine(accel_result.x, accel.min_x, accel.max_x), getZeroToNine(accel_result.y, accel.min_y, accel.max_y), getZeroToNine(gyro_result.x, gyro.min_x, gyro.max_x), getZeroToNine(gyro_result.y, gyro.min_y, gyro.max_y), getZeroToNine(mag_result.x, mag.min_x, mag.max_x), getZeroToNine(mag_result.y, mag.min_y, mag.max_y));
	}
	else if(gear == 'f') {
		printf("P R [D] M%d Speed[%03d%%] Steering_Left[%03d%%] Steering_Right[%03d%%] M2 Only: Accel[%d][%d] Gyro[%d][%d] Mag[%d][%d]\r", manual_state, speed_precent, left_precent, right_precent, getZeroToNine(accel_result.x, accel.min_x, accel.max_x), getZeroToNine(accel_result.y, accel.min_y, accel.max_y), getZeroToNine(gyro_result.x, gyro.min_x, gyro.max_x), getZeroToNine(gyro_result.y, gyro.min_y, gyro.max_y), getZeroToNine(mag_result.x, mag.min_x, mag.max_x), getZeroToNine(mag_result.y, mag.min_y, mag.max_y));
	}
	else if(gear == 'b') {
		printf("P [R] D M%d Speed[%03d%%] Steering_Left[%03d%%] Steering_Right[%03d%%] M2 Only: Accel[%d][%d] Gyro[%d][%d] Mag[%d][%d]\r", manual_state, speed_precent, left_precent, right_precent, getZeroToNine(accel_result.x, accel.min_x, accel.max_x), getZeroToNine(accel_result.y, accel.min_y, accel.max_y), getZeroToNine(gyro_result.x, gyro.min_x, gyro.max_x), getZeroToNine(gyro_result.y, gyro.min_y, gyro.max_y), getZeroToNine(mag_result.x, mag.min_x, mag.max_x), getZeroToNine(mag_result.y, mag.min_y, mag.max_y));
	}
	else {
		printf("Something is wrong\r");
	}*/
}

void *camera_thread(void *param) { // Thread for reading right IR
	int count = 0;
	int temp_steer_percent = 0;
	int speed = 3;
	
	while(CAMERA_STOP_THREAD != 1) {
		int cameraState = captureCamera(count);
		//printf("\nCamera Count: %d State %d\n", count, cameraState);
		if(cameraState == 1) {
			steerRight();
			if(speed > 0) {
				decelerate();
				speed--;
			}
		}
		else if(cameraState == 2) {
			steerLeft();
			if(speed > 0) {
				decelerate();
				speed--;
			}
		}
		else if(cameraState == 0) {
			if(speed < 3) {
				accelerate();
				speed++;
			}
			if(left_precent > 0) { 
				temp_steer_percent = left_precent;
				steerStraight();
				right_precent = temp_steer_percent;
				while(right_precent > 0) {
					steerLeft();
					usleep(25000);
				}
			} 
			else if(right_precent > 0) {
				temp_steer_percent = right_precent;
				steerStraight();
				left_precent = temp_steer_percent;
				while(left_precent > 0) {
					steerRight();
					usleep(25000);
				}
			}
		}
		count += 1;
	}
	CAMERA_STOP_THREAD = 0;
	
	return NULL;
}

void *m_one_accelerometer_thread(void *param) { // Thread for reading center ir
	unsigned long int t1;
	unsigned long int t2;
	struct XYZ result;
	
	t1 = millis();
	result = read_accelerometer( &(io->spi), &(io->gpio) );
	t2 = millis();
	accelerometer.count = 1;
	accelerometer.xyz = malloc(accelerometer.count * sizeof(struct XYZ));
	accelerometer.xyz[accelerometer.count - 1].x = result.x; // bias x
	accelerometer.xyz[accelerometer.count - 1].y = result.y; // bias y
	accelerometer.xyz[accelerometer.count - 1].z = result.z;
	accelerometer.time = malloc(accelerometer.count * sizeof(unsigned long int));
	accelerometer.time[accelerometer.count - 1] = t2 - t1;
	
	do
	{
		t1 = millis();
		result = read_accelerometer( &(io->spi), &(io->gpio) );
		t2 = millis();
		accelerometer.count++;
		accelerometer.xyz = realloc(accelerometer.xyz, accelerometer.count * sizeof(struct XYZ));
		accelerometer.xyz[accelerometer.count - 1].x = result.x;
		accelerometer.xyz[accelerometer.count - 1].y = result.y;
		accelerometer.xyz[accelerometer.count - 1].z = result.z;
		accelerometer.time = realloc(accelerometer.time, accelerometer.count * sizeof(unsigned long int));
		accelerometer.time[accelerometer.count - 1] = t2 - t1;
		// printf("\nAcceleration: %.02f Time: %d\n", accelerometer.xyz[accelerometer.count - 1].x, accelerometer.time[accelerometer.count - 1]);
	} while (m_one_accelerometer_stop_thread != 1);
	
	
	m_one_accelerometer_stop_thread = 0;
	
	return NULL;
}

void *m_two_thread(void *param) {
	do
	{
		accel_result = read_accelerometer( &(io->spi), &(io->gpio) );
		gyro_result = read_gyroscope( &(io->spi), &(io->gpio) );
		mag_result = read_magnetometer( &(io->spi), &(io->gpio) );
		
		if(accel_result.x > accel.max_x) 
		{
			accel.max_x = accel_result.x;
		} 
		else if(accel_result.x < accel.min_x)
		{
			accel.min_x = accel_result.x;
		}
		if(accel_result.y > accel.max_y) 
		{
			accel.max_y = accel_result.y;
		} 
		else if(accel_result.y < accel.min_y)
		{
			accel.min_y = accel_result.y;
		}
		
		if(gyro_result.x > gyro.max_x) 
		{
			gyro.max_x = gyro_result.x;
		} 
		else if(gyro_result.x < gyro.min_x)
		{
			gyro.min_x = gyro_result.x;
		}
		if(gyro_result.y > gyro.max_y) 
		{
			gyro.max_y = gyro_result.y;
		} 
		else if(gyro_result.y < gyro.min_y)
		{
			gyro.min_y = gyro_result.y;
		}
		
		if(mag_result.x > mag.max_x) 
		{
			mag.max_x = mag_result.x;
		} 
		else if(mag_result.x < mag.min_x)
		{
			mag.min_x = mag_result.x;
		}
		if(mag_result.y > mag.max_y) 
		{
			mag.max_y = mag_result.y;
		} 
		else if(mag_result.y < mag.min_y)
		{
			mag.min_y = mag_result.y;
		}
		
		updateTerminal();
		//printf("%d%d %d%d %d%d\n", getZeroToNine(accel_result.x, accel.min_x, accel.max_x), getZeroToNine(accel_result.y, accel.min_y, accel.max_y), getZeroToNine(gyro_result.x, gyro.min_x, gyro.max_x), getZeroToNine(gyro_result.y, gyro.min_y, gyro.max_y), getZeroToNine(mag_result.x, mag.min_x, mag.max_x), getZeroToNine(mag_result.y, mag.min_y, mag.max_y));
		
	} while(m_two_stop_thread != 1);
	
	m_two_stop_thread = 0;
	
	return NULL;
}

int main( void )
{       
	int manual_switch_request = 0; // To keep track if previous key entered was 'm'
	int m_one_imu_active = 0;
		
	pthread_t camera_t;
	pthread_t m_one_accelerometer_t;
	pthread_t m_two_t;
	
	void end_m_one_imu() {
		float velocities[accelerometer.count - 1];
		float avereage_velocity = 0;
		float distance = 0;
		m_one_accelerometer_stop_thread = 1;
		
		if(pthread_join(m_one_accelerometer_t, NULL)) {
			printf("Failed to cancel IR right thread\n");
		}
		velocities[0] = 0;
		
		for(int i = 1; i < accelerometer.count; i++) {
			velocities[i] = velocities[i - 1] + (accelerometer.xyz[i].y - accelerometer.xyz[0].y) * ((float) accelerometer.time[i] / 1000);
			// printf("Velocities at %d at time %.2f\n", i, velocities[i]);
		}

		for(int i = 0; i < accelerometer.count - 1; i++) {
			avereage_velocity += velocities[i];
			distance += velocities[i] * ((float) accelerometer.time[i] / 1000);
			// printf("Distance at %d: %.2f\n", i, distance);
		}
		avereage_velocity = avereage_velocity / ((float) accelerometer.count - 1);
		
		printf("\nAverage Speed: %.2f m/s Distance: %.2f m\n", fabs(avereage_velocity), fabs(distance));
		free(accelerometer.xyz);
		free(accelerometer.time);
		m_one_imu_active = 0;
	}
	
	wiringPiSetupGpio(); // initialize wiringPi.h

	io = import_registers();
	if (io != NULL)
	{
		if(initialize_accelerometer_project() != 0) {
			printf("Could not initalize LSM chip\n");
		}	
		
		/* print where the I/O memory was actually mapped to */
		printf( "mem at 0x%8.8X\n", (unsigned int)io );
		printf( "hit 'ctl c' to quit\n");
		
		/* set the pin function to OUTPUT for GPIO18 - red   LED light */
		io->gpio.GPFSEL0.field.FSEL5 = GPFSEL_OUTPUT;
		io->gpio.GPFSEL0.field.FSEL6 = GPFSEL_OUTPUT;
		io->gpio.GPFSEL2.field.FSEL2 = GPFSEL_OUTPUT;
		io->gpio.GPFSEL2.field.FSEL3 = GPFSEL_OUTPUT;
		io->gpio.GPFSEL2.field.FSEL4 = GPFSEL_INPUT;
		io->gpio.GPFSEL2.field.FSEL5 = GPFSEL_INPUT;
		io->gpio.GPFSEL2.field.FSEL6 = GPFSEL_INPUT;
		
		/* set pwm pins to zero for initial values */\
		if (softPwmCreate(PWMA, left_pwm, pwm_range) != 0) {
			printf("Could not initialize pin PWMA\n");
		}
		if (softPwmCreate(PWMB, right_pwm, pwm_range) != 0) {
			printf("Could not initialize pin PWMB\n");
		}
		
		/* set pins to zero for initial values */
		GPIO_CLR( &(io->gpio), AIN1);
		GPIO_CLR( &(io->gpio), AIN2);
		GPIO_CLR( &(io->gpio), BIN1);
		GPIO_CLR( &(io->gpio), BIN2);
		GPIO_SET(&(io->gpio), DEN_PIN);
		GPIO_SET(&(io->gpio), CS_M_PIN);
		GPIO_SET(&(io->gpio), CS_AG_PIN);

		/* Init max and min values */
		accel.max_x = accelerometer_max_x;
		accel.min_x = accelerometer_min_x;
		accel.max_y = accelerometer_max_y;
		accel.min_y = accelerometer_min_y;
		
		gyro.max_x = gyroscope_max_x;
		gyro.min_x = gyroscope_min_x;
		gyro.max_y = gyroscope_max_y;
		gyro.min_y = gyroscope_min_y;
		
		mag.max_x = magnetometer_max_x;
		mag.min_x = magnetometer_min_x;
		mag.max_y = magnetometer_max_y;
		mag.min_y = magnetometer_min_y;

		printf("Stop=%c Forward=%c Backward=%c Faster=%c Slower=%c Left=%c Right=%c\n", stop, forward, backward, faster, slower, left, right);
		
		while (1)
		{
			if(manual_state == 1) {
				switch(get_pressed_key()) {;
				case stop: // Park
					if(park() != 's') {
						printf("Could not stop\n");
					}	
					if(m_one_imu_active == 1) {
						end_m_one_imu();
					}
					manual_switch_request = 0;		
					break;
				case forward: // Drive
					if(drive() != 'f') {
						printf("Could not drive\n");
					}
					if(m_one_imu_active != 1) {
						if (pthread_create(&m_one_accelerometer_t, NULL, m_one_accelerometer_thread, NULL)) {
							printf("Could not create LSM thread\n");
						}
						m_one_imu_active = 1;
					}
					
					manual_switch_request = 0;
					break;
				case backward: // Reverse
					if(reverse() != 'b') {
						printf("Could not reverse\n");
					}
					if(m_one_imu_active != 1) {
						if (pthread_create(&m_one_accelerometer_t, NULL, m_one_accelerometer_thread, NULL)) {
							printf("Could not create LSM thread\n");
						}
						m_one_imu_active = 1;
					}
					manual_switch_request = 0;
					break;
				case faster: // Accelerate
					accelerate();
					manual_switch_request = 0;
					break;
				case slower: // Decelerate
					decelerate();
					manual_switch_request = 0;
					break;
				case left: // Steer Left
					steerLeft();
					usleep(600000);
					steerStraight();
					manual_switch_request = 0;
					break; 
				case right: // Steer Right
					steerRight();
					usleep(600000);
					steerStraight(); 
					manual_switch_request = 0;
					break;
				case manual: // Change manual
					manual_switch_request = 1;
					break;
				case '2':
					if(manual_switch_request == 1) {
						if(m_one_imu_active == 1) {
							end_m_one_imu();
						}
					
						manual_state = 2;
						if(park() != 's') {
							printf("Could not stop\n");
						}
						steerStraight();
						drive();
						initCamera();
						while(speed_precent < 60 && speed_precent + (100 / speed_steps) <= 60) {
							accelerate();
							usleep(100000);
						}
						if (pthread_create(&camera_t, NULL, camera_thread, NULL)) {
							printf("Could not create camera thread\n");
						}
						if (pthread_create(&m_two_t, NULL, m_two_thread, NULL)) {
							printf("Could not create LSM thread\n");
						}
					}
					manual_switch_request = 0;
					break;
				default:
					manual_switch_request = 0;
					break;
				}
			}
			else if(manual_state == 2) {
				switch(get_pressed_key()) {
				case manual:
					manual_switch_request = 1;
					break;
				case '1':
					if(manual_switch_request == 1) {
						manual_state = 1;
						if(park() != 's') {
							printf("Could not stop\n");
						}
						steerStraight();
						CAMERA_STOP_THREAD = 1;
						m_two_stop_thread = 1;
						if(pthread_join(camera_t, NULL)) {
							printf("Failed to cancel camera thread\n");
						}
						if(pthread_join(m_two_t, NULL)) {
							printf("Failed to cancel IR right thread\n");
						}
						destoryCamera();
					}
					manual_switch_request = 0;
					break;
				default:
					manual_switch_request = 0;
					break;
				}
			}
		}
	}
	else
	{
	; /* warning message already issued */
	}

return 0;
}
 
