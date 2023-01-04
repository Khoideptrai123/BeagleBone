//Assume pins already configured for I2C
// (bbg) $config-pin P9_18 i2c
// (bbg) $config-pin P9_17 i2c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <pthread.h>



#include "sampler.h"


#define I2C_DEVICE_ADDRESS 0x20

// Port 0 register address
// Direction register: control the of the data I/O. 
// When the bit is set, the corresponding pin will become an input. When a bit is clear, the pin will be an output
#define REG_DIRA 0x00 
#define REG_DIRB 0x01

// Output latch register OLAT: povide access to output latches
// A read from this register results in a read of the OLAT and not the port itself
// A write to this register modifies the output latches that modifies the pins configured as outputs
#define REG_OUTA 0x14
#define REG_OUTB 0x15

#define I2CDRV_LINUX_BUS0 "dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "dev/i2c-2"

#define A2D_FILE_VOLTAGE0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095 




int main()
{
    printf("Start sampling...\n");
    Sampler_startSampling();
    printf("Cleaning up...4321\nDone!\n");
return 0;
}
