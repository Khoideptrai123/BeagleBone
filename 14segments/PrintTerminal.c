//Each second, print to the terminal (via printf()) the following
// Line 1:
// # light samples taken during the last second
// The raw value read from the POT
// Number of valid samples in the history
// The averaged light level (from exponential smoothing), displayed as a voltage with 3
// decimal places.
// The number of light level dips that have been found in sample history
// Line 2:
// Display every 200th sample in the sample history.
// Show the oldest of these samples on the left, the newest of these samples on the right

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "potentiometer.h"
#include "sampler.h"

static pthread_t printThread;

static int DipCount =0; //set up number of DIP
static bool isDip = false;
static bool isPrint = true;

void sleepTerminal(long seconds, long nanoseconds){
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}

void manageDip(double sample){
    double avgSenValue = Sampler_getAverageReading(); //from sample.h
    double voltageDiff =  -sample + avgSenValue; //calculate the threshhold
    if (voltageDiff > 0.1){
        isDip = true;
        //exit (0.03V Hysteresis)
        double newSample = getVoltage0Reading();
        voltageDiff = newSample - sample;  
        if(voltageDiff > 0.03 || voltageDiff < -0.03){ 
            isDip = false;
        }  
    }
    else
        isDip = false;
    
     if (isDip)
        DipCount++;

}
int getDips(){
    return DipCount;
}
void* printTerminal_startThread(void* input){ //
    while(isPrint){
        int count = Sampler_getNumSamplesInHistory();
        double potValue = getVoltage0Reading();
        double avgSenValue = Sampler_getAverageReading();
        int ElementCount = Sampler_getHistorySize();
        int length =200;
        double* copy = Sampler_getHistory(length);

        printf ("Average Reading %5.2f\t Pot Value %f\t Dip Count %d\t Sample Count %d\n" , avgSenValue, potValue, DipCount, ElementCount);
        printArray(copy, 200);
        
        sleepTerminal(1,0);
        free(copy);
    }
    return NULL; 
}
void printTerminal_startPrint(void){
    //printf("Start sampling\n");
    pthread_create(&printThread, NULL, &printTerminal_startThread, NULL);
    
}

void printTerminal_stopPrinting(void){
     isPrint = false;
    pthread_join(printThread, NULL);
}