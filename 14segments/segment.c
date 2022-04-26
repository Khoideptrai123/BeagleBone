// Display 14 Seg via I2C
// Assume pins already configured for I2C:
// (bbg)$ config-pin P9_18 i2c
// (bbg)$ config-pin P9_17 i2c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#include "segment.h"

#define I2C_DEVICE_ADDRESS 0x20
#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_OUTA 0x14
#define REG_OUTB 0x15
#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"

static char *DA_LEFT_DIGIT_FILE_NAME = "/sys/class/gpio/gpio61/value";
static char *DA_RIGHT_DIGIT_FILE_NAME = "/sys/class/gpio/gpio44/value";

static long SECOND = 0;
static long NANOSECOND = 8000000;

static bool isSegmentRunning = true;
static int ARR_NUM = 0;

static pthread_t thread;
pthread_mutex_t arrNumMutex = PTHREAD_MUTEX_INITIALIZER;

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

void GPIO_write(char *filename, char *value){
    FILE *pDigitFile = fopen(filename, "w");
    if (pDigitFile == NULL)
    {
        printf("ERROR OPENING %s\n", filename);
        exit(1);
    }
    // int charWritten = 
    fprintf(pDigitFile,"%s", value);
    /*if (charWritten <= 0)
   {
        printf("ERROR WRITING DATA TO GPIO %s\n", filename);
        exit(1);
    }*/
    fclose(pDigitFile);
}

void delay(long DELAY_SECOND, long DELAY_NANOSECOND){
    struct timespec reqDelay = {DELAY_SECOND, DELAY_NANOSECOND};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}

void turnOnLeftDigit(){
    GPIO_write(DA_LEFT_DIGIT_FILE_NAME , "1");
}

void turnOnRightDigit(){
    GPIO_write(DA_RIGHT_DIGIT_FILE_NAME , "1");
}

void turnOffBothDigits(){
    GPIO_write(DA_LEFT_DIGIT_FILE_NAME , "0");
    GPIO_write(DA_RIGHT_DIGIT_FILE_NAME , "0");
}

void displaySingleDigit(int num){
    //num is from 0-9 
    //this function will display both segments with same number
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    writeI2cReg(i2cFileDesc, REG_DIRA, 0x00); // cconfigure GPIO as output
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

    //Reset Display
    writeI2cReg(i2cFileDesc, REG_OUTB, 0x00);
    writeI2cReg(i2cFileDesc, REG_OUTA, 0x00);

    //Display number
    switch (num)
    {
    case 0:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x86);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0xa1);
        break;
    case 1:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x2);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0x80);
        break;
    case 2:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0xe);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0x31);
        break;
    case 3:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x6);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0xb0);
        break;
    case 4:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x8a);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0x90);
        break;
    case 5:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x8c);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0xb0);
        break;
    case 6:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x8c);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0xb1);
        break;
    case 7:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x15);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0x4);
        break;
    case 8:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x8e);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0xb1);
        break;
    case 9:
        writeI2cReg(i2cFileDesc, REG_OUTB, 0x8e);
        writeI2cReg(i2cFileDesc, REG_OUTA, 0x90);
        break;
    }

    // Cleanup I2C access;
    close(i2cFileDesc);
}

void displayNumberViaI2C(int num)
{
    // this function integrates all the command to display the 14 segments
    // num is any number but we only display the last 2 numbers if num >= 100
    pthread_mutex_lock(&arrNumMutex);
    int left_num = num / 10;
    int right_num = num % 10;
    if (num > 99)
    {
        left_num = 9;
        right_num = 9;
    }
    pthread_mutex_unlock(&arrNumMutex);
    turnOffBothDigits();
    
    displaySingleDigit(left_num);
    turnOnLeftDigit();
    delay(SECOND,NANOSECOND);
    turnOffBothDigits();
    
    displaySingleDigit(right_num);
    turnOnRightDigit();
    delay(SECOND,NANOSECOND);
    turnOffBothDigits();
}

void i2c_startDisplay(){
    setupI2C();
    
    pthread_create(&thread, NULL, i2c_display_thread, NULL);
}
void* i2c_display_thread(){
    for (int i = 0; i <100; i++){
        displayNumberViaI2C(i);
        delay(1,0); //delay 1s
    }
    return NULL;
}
void i2c_stopDisplay(){
    turnOffBothDigits();
    isSegmentRunning = false;
    pthread_join(thread, NULL);
}