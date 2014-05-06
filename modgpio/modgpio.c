/*
 * modgpio.c - GPIO device driver for Raspberry Pi rev 2.0 model B
 *
 * Created on: May 6, 2014
 * Author: Vu Nguyen <quangngmetro@gmail.com>
 * License: GPL
 * Modified from Blake Bourque
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>
#include <asm-generic/uaccess.h>
#include <linux/types.h>
#include <linux/sched.h>

#include "modgpio.h"

// Prefix "rpigpio_ / RPIGPIO_" is used in this module to avoid name pollution
#define RPIGPIO_MOD_AUTH 	"Vu Nguyen"
#define RPIGPIO_MOD_DESC 	"GPIO device driver for Raspberry Pi"
#define RPIGPIO_MOD_SDEV 	"Raspberry Pi rev 2.0 model B"
#define RPIGPIO_MOD_NAME 	"rpigpio"

#define PIN_RESERVED 	1 	// reserved pins for system use
#define PIN_FREE 		0 	// available pins for request
#define PIN_ARRAY_LEN 32

static int rpigpio_open(struct inode *inode, struct file *filp);
static int rpigpio_release(struct inode *inode, struct file *filp);
static long rpigpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

// Global variables
static int deviceOpenCounter = 0;
struct rpigpio_dev {
	int 			mjr;
	struct class 	*cls;
	spinlock_t 		lock;
	uint32_t 		pin_state_arr[PIN_ARRAY_LEN];
	PIN_DIRECTION_t pin_dir_arr[PIN_ARRAY_LEN];
};

static struct rpigpio_dev std = {
		.mjr = 0,
		.cls = NULL,
};

static const struct file_operations rpigpio_fops = {
		.owner	= 			THIS_MODULE,
		.open	= 			rpigpio_open,
		.release = 			rpigpio_release,
		.unlocked_ioctl = 	rpigpio_ioctl,
};

// Implementation of entry points
static int
rpigpio_open(struct inode*inode, struct file *filp)
{
	spin_lock(&std.lock);
	deviceOpenCounter++;
	spin_unlock(&std.lock);
	try_module_get(THIS_MODULE);

	return 0;
}

static int
rpigpio_release(struct inode *inode, struct file *filp)
{
	int i = 0;

	spin_lock(&std.lock);
	deviceOpenCounter--;
	if (deviceOpenCounter == 0) {
		for (i = 0; i < PIN_ARRAY_LEN; i++) {
			if (i != 0 && i != 1 && i != 5 && i != 6 &&
				i != 12 && i != 13 && i != 16 && i != 19 &&
				i != 20 && i != 21 && i != 26) {
				printk(KERN_DEBUG "[FREE] Pin:%d\n", i);
				std.pin_state_arr[i] = PIN_FREE;
			}
		}
	}
	spin_unlock(&std.lock);
	module_put(THIS_MODULE);

	return 0;
}

static long
rpigpio_ioctl(	struct file *filp, unsigned int cmd, unsigned long arg)
{
	int pin;
	unsigned long ret;
	int retval;
	uint8_t val;
	struct gpio_data_write wdata;
	struct gpio_data_mode mdata;

	switch (cmd) {
	case GPIO_REQUEST:
		get_user (pin, (int __user *) arg);
		spin_lock(&std.lock);
		if (pin > PIN_ARRAY_LEN || pin < 0 || std.pin_state_arr[pin] == PIN_RESERVED) {
			spin_unlock(&std.lock);
			return -EFAULT;
		} else if (std.pin_state_arr[pin] != PIN_FREE) {
			spin_unlock(&std.lock);
			return -EBUSY;
		}
		std.pin_state_arr[pin] = current->pid;
		spin_unlock(&std.lock);
		printk(KERN_DEBUG "[REQUEST] Pin:%d Assn To:%d\n", pin, current->pid);
		return 0;

	case GPIO_FREE:
		get_user (pin, (int __user *) arg);
		spin_lock(&std.lock);
		if (pin > PIN_ARRAY_LEN || pin < 0 || std.pin_state_arr[pin] == PIN_RESERVED) {
			spin_unlock(&std.lock);
			return -EFAULT;
		} else if (std.pin_state_arr[pin] != current->pid) {
			spin_unlock(&std.lock);
			return -EACCES;
		}
		std.pin_state_arr[pin] = PIN_FREE;
		spin_unlock(&std.lock);
		printk(KERN_DEBUG "[FREE] Pin:%d From:%d\n", pin, current->pid);
		return 0;

	case GPIO_MODE:
		ret = copy_from_user(&mdata, (struct gpio_data_mode __user *)arg, sizeof(struct gpio_data_mode));
		if (ret != 0) {
			printk(KERN_DEBUG "[MODE] Error copying data from userspace\n");
			return -EFAULT;
		}
		spin_lock(&std.lock);
		if (mdata.pin > 31 || mdata.pin < 0 || std.pin_state_arr[mdata.pin] == PIN_RESERVED) {
			spin_unlock(&std.lock);
			return -EFAULT;
		} else if (std.pin_state_arr[mdata.pin] != current->pid) {
			spin_unlock(&std.lock);
			return -EACCES;
		}

		if(mdata.data == MODE_INPUT) {
			retval = gpio_direction_input(mdata.pin);
			if (retval < 0) {
				spin_unlock(&std.lock);
				return retval;
			}
			std.pin_dir_arr[mdata.pin] = DIRECTION_IN;
			printk(KERN_DEBUG "[MODE] Pin %d set as Input\n", mdata.pin);
		} else if (mdata.data == MODE_OUTPUT) {
			retval = gpio_direction_output(mdata.pin, 1);
			if (retval < 0) {
				spin_unlock(&std.lock);
				return retval;
			}
			std.pin_dir_arr[mdata.pin] = DIRECTION_OUT;
			printk(KERN_DEBUG "[MODE] Pin %d set as Output\n", mdata.pin);
		} else {
			spin_unlock(&std.lock);
			return -EINVAL;
		}
		spin_unlock(&std.lock);
		return 0;

	case GPIO_READ:
		get_user (pin, (int __user *) arg);
		val = gpio_get_value(pin);
		printk(KERN_DEBUG "[READ] Pin: %d Val:%d\n", pin, val);
		put_user(val, (uint8_t __user *)arg);
		return 0;

	case GPIO_WRITE:
		ret = copy_from_user(&wdata, (struct gpio_data_write __user *)arg, sizeof(struct gpio_data_write));
		if (ret != 0) {
			printk(KERN_DEBUG "[WRITE] Error copying data from userspace\n");
			return -EFAULT;
		}
		spin_lock(&std.lock);
		if (std.pin_state_arr[wdata.pin] != current->pid) {
			spin_unlock(&std.lock);
			return -EACCES;
		}
		if (std.pin_dir_arr[wdata.pin] == DIRECTION_IN) {
			printk(KERN_DEBUG "Cannot set Input pin\n");
			spin_unlock(&std.lock);
			return -EACCES;
		}
		if (wdata.data == 1)
			gpio_set_value(wdata.pin, 1);
		else
			gpio_set_value(wdata.pin, 0);
		spin_unlock(&std.lock);
		printk(KERN_INFO "[WRITE] Pin: %d Val:%d\n", wdata.pin, wdata.data);

		return 0;

	case GPIO_TOGGLE:
		get_user (pin, (int __user *) arg);
		spin_lock(&std.lock);
		if (pin > PIN_ARRAY_LEN || pin < 0 || std.pin_state_arr[pin] == PIN_RESERVED) {
			spin_unlock(&std.lock);
			return -EFAULT;
		} else if (std.pin_state_arr[pin] != current->pid) {
			spin_unlock(&std.lock);
			return -EACCES;
		}
		if (std.pin_dir_arr[pin] == DIRECTION_IN) {
			printk(KERN_DEBUG "Cannot set Input pin\n");
			spin_unlock(&std.lock);
			return -EACCES;
		}
		val = gpio_get_value(pin);
		if (val > 0) {
			gpio_set_value(pin, 0);
		} else {
			gpio_set_value(pin, 1);
		}
		put_user(val?0:1, (uint8_t __user *)arg);
		spin_unlock(&std.lock);
		printk(KERN_DEBUG "[TOGGLE] Pin:%d From:%.1d To:%.1d\n", pin, val, val?0:1);

		return 0;

	default:
		return -ENOTTY;
	}
}

// Sets permissions for device file
static char *st_devnode(struct device *dev, umode_t *mode)
{
	if (mode) *mode = 0666;//add a leading 0 makes number octal
	return NULL;
}

static int __init
rpigpio_minit(void)
{
	int i = 0;
	int retval;
	struct device *dev;

	// Register char device
	std.mjr = register_chrdev(0, RPIGPIO_MOD_NAME, &rpigpio_fops);
	if (std.mjr < 0) {
		printk(KERN_ALERT "[gpio] Cannot Register");
		return std.mjr;
	}
	printk(KERN_INFO "[gpio] Major #%d\n", std.mjr);

	// Create class in /sys directory
	std.cls = class_create(THIS_MODULE, "std.cls");
	if (IS_ERR(std.cls)) {
		printk(KERN_ALERT "[gpio] Cannot get class\n");
		unregister_chrdev(std.mjr, RPIGPIO_MOD_NAME);
		return PTR_ERR(std.cls);
	}

	std.cls->devnode = st_devnode;

	// Create device file in /dev directory
	dev = device_create(std.cls, NULL, MKDEV(std.mjr, 0), (void*)&std, RPIGPIO_MOD_NAME);
	if (IS_ERR(dev)) {
		printk(KERN_ALERT "[gpio] Cannot create device\n");
		class_destroy(std.cls);
		unregister_chrdev(std.mjr, RPIGPIO_MOD_NAME);
		return PTR_ERR(dev);
	}

	// Initialize the spinlock
	spin_lock_init(&(std.lock));

	// Initialize the per-device structure
	for (i = 0; i < PIN_ARRAY_LEN; i++) {
		if (i != 0 && i != 1 && i != 5 && i != 6 &&
			i != 12 && i != 13 && i != 16 && i != 19 &&
			i != 20 && i != 21 && i != 26) {
			std.pin_state_arr[i] = PIN_FREE;
			retval = gpio_request(i, NULL);
			if (retval < 0)
				return retval;
		} else {
			std.pin_state_arr[i] = PIN_RESERVED;
		}
		std.pin_dir_arr[i] = DIRECTION_OUT;
	}
	printk(KERN_INFO "[gpio] %s Installed\n", RPIGPIO_MOD_NAME);

	return 0;
}

static void __exit
rpigpio_mcleanup(void)
{
	int i = 0;

	for (i = 0; i < PIN_ARRAY_LEN; i++) {
		if (i != 0 && i != 1 && i != 5 && i != 6 &&
			i != 12 && i != 13 && i != 16 && i != 19 &&
			i != 20 && i != 21 && i != 26) {
			std.pin_state_arr[i] = PIN_FREE;
			gpio_free(i);
		}
	}
	device_destroy(std.cls, MKDEV(std.mjr, 0));
	class_destroy(std.cls);
	unregister_chrdev(std.mjr, RPIGPIO_MOD_NAME);

	printk(KERN_NOTICE "[gpio] Removed\n");
}

module_init(rpigpio_minit);
module_exit(rpigpio_mcleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(RPIGPIO_MOD_AUTH);
MODULE_DESCRIPTION(RPIGPIO_MOD_DESC);
MODULE_SUPPORTED_DEVICE(RPIGPIO_MOD_SDEV);


