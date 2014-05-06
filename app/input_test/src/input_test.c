/*
 ============================================================================
 Name        : input_test.c
 Author      : Vu Nguyen <quangngmetro@gmail.com>
 Version     : 1.0
 Copyright   : GPL
 Description : This application program was developed to test the input
 functionality of the GPIO device driver implemented for Raspberry Pi rev 2.0
 model B platform.
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

#define NUM_PINS 17

static int pin[NUM_PINS] = {
		2,
		3,
		4,
		7,
		8,
		9,
		10,
		11,
		14,
		15,
		17,
		18,
		22,
		23,
		24,
		25,
		27,
};

int main(int argc, char * argv[])
{
	int i;
	int fd;
	int ret;
	int t;
	int val;
	struct gpio_data_mode modeData;

	fd = open("/dev/rpigpio", O_RDONLY);
	if (!fd) {
		perror("open(O_RDONLY)");
		return errno;
	}

	// Reserve GPIO pins
	for (i = 0; i < NUM_PINS; i++) {
		t = pin[i];
		ret = ioctl(fd, GPIO_REQUEST, &t);
		if (ret < 0)
			perror("ioctl");
	}
	printf("Reserved %d GPIO pins\n", i);

	// Set GPIO pins as input
	for (i = 0; i < NUM_PINS; i++) {
		t = pin[i];
		modeData.pin = t;
		modeData.data = MODE_INPUT;
		ret = ioctl(fd, GPIO_MODE, &modeData);
		if (ret < 0)
			perror("ioctl");
	}
	printf("Set %d GPIO pins as input\n", i);
	
	// Read the state of GPIO pins
	for (i = 0; i < NUM_PINS; i++) {
		t = pin[i];
		val = t;
		ret = ioctl(fd, GPIO_READ, &val);
		if (ret < 0)
			perror("ioctl");
		else
			printf("Pin %d value=%d\n", t, val);
	}

	return 0;
}
