/*
 * modgpio.h
 *
 *  Created on: May 5, 2014
 *      Author: quangng
 */

#ifndef MODGPIO_H_
#define MODGPIO_H_

#define GPIO_IOC_MAGIC 'k'

typedef enum {MODE_INPUT=0, MODE_OUTPUT} PIN_MODE_t;
typedef enum {DIRECTION_IN = 0, DIRECTION_OUT} PIN_DIRECTION_t;

struct gpio_data_write {
	int pin;
	char data;
};

struct gpio_data_mode {
	int pin;
	PIN_MODE_t data;
};
//in: pin to read //out: value //the value read on the pin
#define GPIO_READ _IOWR(GPIO_IOC_MAGIC, 0x90, int)

//in: struct(pin, data) //out: NONE
#define GPIO_WRITE _IOW(GPIO_IOC_MAGIC, 0x91, struct gpio_data_write)

//in: pin to request //out: success/fail // request exclusive modify privileges
#define GPIO_REQUEST _IOW(GPIO_IOC_MAGIC, 0x92, int)

//in: pin to free
#define GPIO_FREE _IOW(GPIO_IOC_MAGIC, 0x93, int)

//in: pin to toggle //out: new value
#define GPIO_TOGGLE _IOWR(GPIO_IOC_MAGIC, 0x94, int)

//in: struct (pin, mode[i/o])
#define GPIO_MODE _IOW(GPIO_IOC_MAGIC, 0x95, struct gpio_data_mode)

#endif /* MODGPIO_H_ */
