/*
 ============================================================================
 Name        : direction_collision_test.c
 Author      : Vu Nguyen <quangngmetro@gmail.com>
 Version     : 1.0
 Copyright   : GPL
 Description : This application program was deleloped to test the feature of
 the GPIO device driver that it does not allow a process to set the state of
 an input GPIO pin.
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>


#include "modgpio.h"

int main(int argc, char * argv[])
{
	int fd;
	int ret;
	int t = 27;
	struct gpio_data_mode modeData;
	struct gpio_data_write writeData;

	fd = open("/dev/rpigpio", O_RDONLY);
	if (!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}
	//reserve pin
	ret = ioctl(fd, GPIO_REQUEST, &t);
	if (ret < 0)
		perror("ioctl");
	else
		printf("Reserved pin %d\n", t);

	//set output
	modeData.pin = t;
	modeData.data = MODE_OUTPUT;
	ret = ioctl(fd, GPIO_MODE, &modeData);
	if (ret < 0)
		perror("ioctl");
	else
		printf("Set pin %d as OUTPUT\n", t);

	// set pin to high state
	writeData.pin = t;
	writeData.data = 1;
	ret = ioctl(fd, GPIO_WRITE, &writeData);
	if (ret < 0)
		perror("ioctl");
	else
		printf("Set pin to high state\n");
	sleep (3);

	//set input
	modeData.pin = t;
	modeData.data = MODE_INPUT;
	ret = ioctl(fd, GPIO_MODE, &modeData);
	if (ret < 0)
		perror("ioctl");
	else
		printf("Set pin %d as INPUT\n", t);
	
	// set pin to high state from input pin to test direction collision
	writeData.pin = t;
	writeData.data = 1;
	ret = ioctl(fd, GPIO_WRITE, &writeData);
	if (ret < 0)
		perror("ioctl");
	else
		printf("Set pin to high state\n");

	sleep (3);
	return 0;
}
