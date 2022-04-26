// sampler.h
//Module to sample light levels in the background (thread).
//It provides the average light level and number of samples taken
#ifndef _SAMPLER_H_
#define _SAMPLER_H_



//Begin /end the background thread which samples light levels
void Sampler_startSampling(void);
void Sampler_stopSampling(void);

//Set/get the maximum number of samples to store in the history

void Sampler_setHistorySize(int newSize);
int Sampler_getHistorySize(void);

//Get a copy of the samples in the sample history
//Returns a newly allocated array and set 'length' to be the
// number of elements in the returned array (output-only parameter)
// The calling code must call free() on the returned pointer
// Note: privde both data and size to ensure consistency

double* Sampler_getHistory(int length);

//Return how many samples are currently in the history
// may be less than the history size of the history is not yet full
int Sampler_getNumSamplesInHistory();

//get the average light level(not tied to history)
double Sampler_getAverageReading(void);

//Get the total number of light level taken so far.
long long Sampler_getNumSamplesTaken(void);



#endif