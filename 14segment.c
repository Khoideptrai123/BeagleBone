//Assume pins already configured for I2C
// (bbg) $config-pin P9_18 i2c
// (bbg) $config-pin P9_17 i2c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

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

void setupI2C()
{
    //Config pin
    system("config-pin P9_18 i2c");
    system("config-pin P9_17 i2c");
    //Config GPIO
    system("echo 61 > /sys/class/gpio/export");
    system("echo 44 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio61/direction");
    system("echo out > /sys/class/gpio/gpio44/direction");
}


static int initI2cBus(char* bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR); //o_RDWR: read and write
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result <0){
        perror("I2C: Unable to set I2C device to slave address");
        exit(1);
    }
    return i2cFileDesc;
}

//write a register
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if(res != 2){
        perror("I2C: Unable to write i2c register");
        exit(1);
    }
}

//read a register
static unsigned char readI2cReg(int i2cFileDesc,unsigned char regAddr)
{
    //To read a register, must fist write the address
    int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));
    if(res != sizeof(regAddr)){
        perror("I2C: Unable to write to I2C register");
        exit(1);
    }
    //now read the value and return it
    char value =0;
    res = read(i2cFileDesc, &value, sizeof(value));
    if(res != sizeof(regAddr)){
        perror("I2C: Unable to read from I2C register");
        exit(1);
    }
    return value;
}

int main()
{
    printf("Drive display (assume GPIO #61 and #44 are output and 1)\n");
    setupI2C();
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

     writeI2cReg(i2cFileDesc, REG_DIRA, 0x00); // cconfigure GPIO as output
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

    //drive an hour-glass looking character
    // Like an X with a bar on top & bottom
    writeI2cReg(i2cFileDesc, REG_DIRA, 0x2A); // cconfigure GPIO as output
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x54);

    //read a register
    unsigned char regVal = readI2cReg(i2cFileDesc, REG_OUTA);
    printf("REG OUT-A = 0x%02x\n", regVal); 
    
    //clean up I2C address
    close(i2cFileDesc);
    return 0;
}
