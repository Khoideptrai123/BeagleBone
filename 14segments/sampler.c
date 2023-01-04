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
#include "sampler.h"
#include "segment.h"
#include "udp.h"

#define A2D_FILE_VOLTAGE1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define EXPONENTIAL_WEIGHT 0.999
#define DIP_RANGE 0.1
#define HYSTERESIS 0.03
#define NUM_THREAD 6

static pthread_t thread[NUM_THREAD];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t condArrayAvailable;
static pthread_cond_t potCond = PTHREAD_COND_INITIALIZER;


static int BUFFER_SIZE = 1000;
static int count =-1; // number of samples taken in the last second
static int DipCount =0;
static double lightLevelAverage = -1;
static bool isRunning =  true;
static int currentPotVal =-1; // un-initalize
long long historicCount = 0;


circular_buf_t* history;


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

static void *displaySampleStatus(void *buffer) {
	//circular_buf_t *bufferPointer =  buffer;
    printf("Start thread 3\n");
	while(isRunning) {
		//Critical Section
		pthread_mutex_lock(&mutex);
		{
			if(currentPotVal <= -1) { //Halt thread until Pot value is initialized
				pthread_cond_wait(&potCond, &mutex); 
			}
			printf("Samples/s = %d  Pot Value = %d Array Size = %d Capacity = %d Element Count = %d  avg = %0.3f  dips = %d\n", 
				count, 
				currentPotVal, 
                getArraySize(currentPotVal),
                circular_buf_capacity(history),
                circular_buf_getCount(history),
				(lightLevelAverage < 0) ? 0:lightLevelAverage, 
				DipCount
			);
			
			count = 0;
		}
		pthread_mutex_unlock(&mutex);
        //displayBuffer(history, 500); //prints every 200th samples
		sleep(1, 0); //Sleep for 1 second
	}
	return NULL;
}

static void *readPotVal(void *buffer) {
	//circular_buf_t *bufferPointer =  buffer;
    printf("Start thread 2\n");
	while(isRunning) {
		//Critical section
		pthread_mutex_lock(&mutex);
		{
			currentPotVal = getArraySize(getVoltage0Reading()); //Read pot value
			//if pot value is zero set to 1. Otherwise, actually pot value
			int updatingPotVal = currentPotVal > 0 ? currentPotVal : 1;
			//Set buffer value according to the new pot value
			resizeBuffer(history, updatingPotVal);
		}

		//Let readLightSensorVal() and displaySampleStatus() continue 
		//if they are on hold
		pthread_cond_signal(&potCond);
		pthread_mutex_unlock(&mutex);

		sleep(1, 0); //Sleep for 1 second
	}
	return NULL;
}

static void* Sampler_StartThread(void *buffer){  
   // printf("Init circular buffer\n");
    // starting all required thread
    //circular_buf_t* bufferPointer = buffer;
    printf("Start thread 1 \n");
    lightLevelAverage = getVoltage1Reading(); //init the value of the average
    while (isRunning)
    {
        //Critical section
        pthread_mutex_lock(&mutex);
       {
            if(currentPotVal <= -1) { //If pot value has not initialized yet, halt the function
				pthread_cond_wait(&potCond, &mutex); 
			}

            //read the light sensor value
            double lightSensor = getVoltage1Reading(); 
            circular_buf_try_put(history, lightSensor); //putting data to the circular buffer
            historicCount++;

            if(lightLevelAverage == -1){
                lightLevelAverage= lightSensor;
            }
            else{
                //filter the sensor value using exponential smoothing
                //weights the previous average at 99.9%
                lightLevelAverage = (lightLevelAverage* EXPONENTIAL_WEIGHT)+ (lightSensor*(1-EXPONENTIAL_WEIGHT));
            }
    
        }
        
        count++;
        pthread_mutex_unlock(&mutex);
        sleep(0,1000000); // wait for 1ms between each reading 
            
    } 
    return NULL; 

}
static void* countLightDips(void* buffer){
    //circular_buf_t* bufferPointer = buffer;
    printf("Start thread 4\n");

    while(isRunning){
        //DipCount =0;
        pthread_mutex_lock(&mutex);
        {
            // if light level average is not initialized, no need to count dips
            if(lightLevelAverage <= -1){
                pthread_cond_wait(&potCond, &mutex); 
            }    
            else {
                int historySize  = circular_buf_getCount(history);
                
                //double currentLightAverage = lightLevelAverage; // initialized the current average

                //get copy of current sample in a new buffer
                double currentLightSample[historySize];
                circular_buf_returnBuf(history, currentLightSample);

                int newDipCounts = 0;
                bool canDip = true;
               
                 
                // for(int i  = 0; i < historySize; i++){// check for Dip condition in the new array sample
                //     if(canDip){
                //         // dip is detected when the (average - sample value) > 0.1
                //         if(currentLightAverage - currentLightSample[i] >= DIP_RANGE){
                //             canDip =  false; 
                //             newDipCounts++;
                //             continue;
                //         } 
                //         //printf("DITMEMAY: %d", DipCount);
                //         // Apply Hysteresis to check whether samples can be checked for dips in the next iteration
                //         if(!canDip && (currentLightSample[i] > currentLightAverage+ HYSTERESIS)){
                //             canDip = true;
                //         }
                //     }
                // }
                for (int i =0; i < historySize; i++){
                    double diff = lightLevelAverage - currentLightSample[i];
                    if(diff>= DIP_RANGE){
                        if(canDip){
                            newDipCounts++;
                            canDip = false;
                        }
                    }else if(diff < HYSTERESIS){
                        // If sample voltage > history + 0.03V (hysteresis) => Un-dipped
                        canDip = true;
                    }
                }
                DipCount = newDipCounts; 
            }
        }
        pthread_mutex_unlock(&mutex);
        sleep(0, 1000000); // sleep for 0.1s
    }
    return NULL;
}
/*
Thread function to display the current dip count
input: *void
output: *void
*/
static void* displayLightDip(){
    printf("Start thread 5\n");
    setupI2C(); // initialize the I2C pin
    while(isRunning){
        displayNumberViaI2C(DipCount); // display the Dipcount via the 14 segments
    }
    turnOffBothDigits(); // stop the display
    return NULL;
}

/*
Samples light level voltage
Features:
	 - samples light level voltage
	 - read value from potentiometer to update sample history size
	 - calculate exponential average
	 - detect dips in light sensor
	 - display number of dips in 14 seg display
	 - UDP connection on port 12345 for remote query on samples
	 	- commands: help, count, length, get N, history, dips, stop
	 - uses 6 threads
input: void
output: void
*/

void Sampler_startSampling(void){
    //printf("Start sampling\n");
    history = circ_buf_init(BUFFER_SIZE);
    Udp_setup();

    pthread_create(&thread[0], NULL, Sampler_StartThread, &history); // Thread to start reading the light sensor value
    
    pthread_create(&thread[1], NULL, readPotVal, &history); // Thread to start reading the potentiometer value
    
    pthread_create(&thread[2], NULL, displaySampleStatus, &history); // Thread to display the sample status
    
    pthread_create(&thread[3], NULL, countLightDips, &history); // Thread to count the light dip in the sample

    pthread_create(&thread[4], NULL, displayLightDip, NULL); // Thread to display light dip in the sample

    pthread_create(&thread[5], NULL, Udp_threadCommandHandling, &history); // Thread to handle UDP communication



    //i2c_startDisplay(); 
   

    //wait until threads are done and clean up memories by thread
    for(int i = 0; i < 6; i++) {
		pthread_join(thread[i], NULL);
	}

    // Free the deallocated array in the buffer
    circular_buf_free(history);
}




// void Sampler_stopSampling(void){
//     isRunning = false;
//     pthread_join(thread, NULL);

//     //circular_buf_free(history);
//     if (history)
//         free(history);
// }//int samples[1000];



int Sampler_getHistorySize(void){
    return circular_buf_capacity(history);
}

// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `length` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: provides both data and size to ensure consistency.
double* Sampler_getHistory(int* length){
    double* ret_val = NULL;
    *length = 0;
    pthread_mutex_lock(&mutex);
    {
    int historySize = Sampler_getNumSamplesInHistory();
    // double currentHistory[historySize];
    // circular_buf_returnBuf(history, currentHistory);
    ret_val = malloc(sizeof(double)*(historySize));
    circular_buf_returnBuf(history, ret_val);
    }
    pthread_mutex_unlock(&mutex);
  
    return ret_val;
}
int Sampler_getNumSamplesInHistory(){
    return circular_buf_getCount(history);
}

long long Sampler_getNumSamplesTaken(void){
    return historicCount;
}

int Sampler_getDips(){
    return DipCount;
}

// void printArray(double* arr, int n){
// 	for(int i = 0; i < n; i++) {
// 			printf("%f ", arr[i]);
// 		}
// }
