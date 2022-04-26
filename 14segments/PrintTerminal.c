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

void manageDip(){

}