// Reading the Potentiometer on the BBG Cape
// The potentiometer is connected to AIN0
// (bbg)$ config-pin P9_18 i2c
// (bbg)$ config-pin P9_17 i2c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "potentiometer.h"
#include "circularbuffer.h"
#define A2D_FILE_VOLTAGE1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

static pthread_t thread;
pthread_mutex_t arrNumMutex_Buf = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condArrayAvailable;


static int BUFFER_SIZE = 10;
static long long count =0;
static double sensorVFiltered =0;
int DipCount =0;
static bool isDip = false;

circular_buf_t* history = NULL;




static double *return_arr;


static bool isRunning =  true;
void sleep(long seconds, long nanoseconds){
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}

double getVoltage1Reading(){
    //Open File
    FILE *f = fopen(A2D_FILE_VOLTAGE1, "r");
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
    double voltage = ((double)a2dReading/4095)*1.8;
    return voltage;
}
double Sampler_getAverageReading(void){
    double LightVoltage = getVoltage1Reading();
    double a = 0.99;
    sensorVFiltered = (1-a)*LightVoltage + a*sensorVFiltered;
    return sensorVFiltered;
}

void ManageDipDetector(double sample){
    double voltage =  sample - sensorVFiltered;
    if (voltage > 0.1){
        isDip = true;
    }
    //exit (0.03V Hysteresis)
    double newSample = getVoltage1Reading();
    double newAverage = Sampler_getAverageReading();
    voltage = newSample -newAverage;
    
    if(voltage < 0.07){ 
        isDip = false;
    }
    if (isDip)
        DipCount++;
}

void* Sampler_StartThread(void *input){
    printf("Init circular buffer\n");
    history = circ_buf_init(BUFFER_SIZE);
    sensorVFiltered = getVoltage1Reading(); //init the value of the average
    while (isRunning)
    {
        double lightSensor = getVoltage1Reading(); // read the light sensor value
        circular_buf_put(history, lightSensor);
        sensorVFiltered = Sampler_getAverageReading();
        ManageDipDetector(lightSensor);
        double data;
        int ElementCount = circular_buf_getCount(history);
        //circular_buf_get(history, &data);
        
        printf ("Light sensor %5.2f\t Average Reading %5.2f\t Dip Count %d Sample Count %d\n" , lightSensor, sensorVFiltered, DipCount, ElementCount);


        count++;
        sleep(1,0);
        //printf("Number of samples taken %lld\n", count);
            
    }
    return NULL; 

}

void Sampler_startSampling(void){
    printf("Start sampling\n");
    isRunning = true;
    pthread_create(&thread, NULL, &Sampler_StartThread, NULL);
    //init the circular buffer
    
}

void Sampler_stopSampling(void){
    // isRunning = false;
    pthread_join(thread, NULL);

    circular_buf_free(history);
    if (history)
        free(history);
}//int samples[1000];

void Sampler_setHistorySize(int newSize)
{
    int currentSize = circular_buf_capacity(history);
    pthread_mutex_lock(&arrNumMutex_Buf);
    {
        circular_buf_setCapacity(history, newSize);
    }
    pthread_mutex_unlock(&arrNumMutex_Buf);
    // if (currentSize> newSize){
    //     return_arr = Sampler_getHistory(newSize);
    // }
}

int Sampler_getHistorySize(void){
    return circular_buf_capacity(history);
}

double* Sampler_getHistory(int length){
    // while (history->buffer == NULL) {
    //     pthread_cond_wait(&condArrayAvailable, &arrNumMutex);
    // }
    
    return_arr = circular_buf_returnBuf(history, length);
    return return_arr;

}

long long Sampler_getNumSamplesTaken(void){
    return count;
}

