MODULE_NAME = modgpio
KDIR = /home/quangng/study/linux_device_drivers/Metropolia/labs/final_year_project/ldd_raspi_gpio_driver/kernel/Rpi_kernel/linux
TOOLCHAIN = /home/quangng/study/linux_device_drivers/Metropolia/labs/final_year_project/ldd_raspi_gpio_driver/buildtools/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
TARGET = arm
PWD := $(shell pwd)
MODULE_UPLOAD = $(MODULE_NAME).ko


obj-m := $(MODULE_NAME).o

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) modules
	
install:
	scp $(MODULE_UPLOAD) pi@quangng.homenet.org:/home/pi/linux_device_driver/kernel_modules/gpio_device_driver/

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(TARGET) CROSS_COMPILE=$(TOOLCHAIN) clean