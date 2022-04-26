// Reading the Potentiometer on the BBG Cape
// The potentiometer is connected to AIN0
// (bbg)$ config-pin P9_18 i2c
// (bbg)$ config-pin P9_17 i2c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "potentiometer.h"

#define A2D_FILE_VOLTAGE0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095 

static int ARRAY_SIZE_TABLE[] = {1, 20, 60, 120, 250, 300, 500, 800, 1200, 2100};
static int A2D_READING_TABLE[] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4100};

static int A2D_INCREMENT = 500;

int getVoltage0Reading(){
    //Open File
    FILE *f = fopen(A2D_FILE_VOLTAGE0, "r");
    if(!f){
        printf("ERROR: Unable to open voltage input file. Cape Loaded?\n");
        printf("       Check /boot/uEnv.txt for correct options. \n");
        exit(-1);
    }
    // Get Reading
    int a2dReading =0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0){
        printf("ERROR: Unable to read value from voltage input file.\n");
        exit(-1);
    }

    //close file
    fclose(f);
    return a2dReading;
}
int getArraySize(int A2D_value){
    int index = A2D_value / A2D_INCREMENT;
    int A2D_lowerBound = A2D_READING_TABLE[index];
    int A2D_upperBound = A2D_READING_TABLE[index + 1];
    int ArraySize_lowerBound = ARRAY_SIZE_TABLE[index];
    int ArraySize_upperBound = ARRAY_SIZE_TABLE[index + 1];
    int ArraySize_value = ((double)(A2D_value - A2D_lowerBound) / (double)(A2D_upperBound - A2D_lowerBound)) * (ArraySize_upperBound - ArraySize_lowerBound) + ArraySize_lowerBound;
    return ArraySize_value;
}