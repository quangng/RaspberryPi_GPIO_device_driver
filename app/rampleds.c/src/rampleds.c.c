/*
 ============================================================================
 Name        : output_test.c
 Author      : Vu Nguyen <quangngmetro@gmail.com>
 Version     : 1.0
 Copyright   : GPL
 Description : This application program was developed to test the output
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

#define NUM_PINS 21

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
		28,
		29,
		30,
		31,
};


int main(int argc, char * argv[])
{
	int i;
	int fd;
	int ret;
	int t;
	struct gpio_data_mode modeData;
	struct gpio_data_write writeData;

	fd = open("/dev/rpigpio", O_RDWR);
	if (!fd) {
		perror("open(O_RDWR)");
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

	// Set the direction of GPIO pins as output
	for (i = 0; i < NUM_PINS; i++) {
		t = pin[i];
		modeData.pin = t;
		modeData.data = MODE_OUTPUT;
		ret = ioctl(fd, GPIO_MODE, &modeData);
		if (ret < 0)
			perror("ioctl");
	}
	printf("Set %d GPIO pins as OUTPUT\n", i);
	
	// Set the state of GPIO pins as high
	for (i = 0; i < NUM_PINS; i++) {
		t = pin[i];
		writeData.pin = t;
		writeData.data = 1;
		ret = ioctl(fd, GPIO_WRITE, &writeData);
		if (ret < 0)
			perror("ioctl");
	}
	printf("Set %d GPIO pins as HIGH\n", i);
	sleep(1);

	// Set the state of GPIO pins as low
	for (i = 0; i < NUM_PINS; i++) {
		t = pin[i];
		writeData.pin = t;
		writeData.data = 0;
		ret = ioctl(fd, GPIO_WRITE, &writeData);
		if (ret < 0)
			perror("ioctl");
	}
	printf("Set %d GPIO pins as LOW\n", i);

	return 0;
}
