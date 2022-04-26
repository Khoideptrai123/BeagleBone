#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include"circularbuffer.h"

struct circular_buf_t{
    double* buffer;
	int head;
	int tail;
	int capacity; // of the buffer
    //int count; // count the number of elements in the buffer
	bool full;

};


circular_buf_t *circ_buf_init(int size){
    circular_buf_t *buffer = malloc(sizeof(circular_buf_t));
    buffer->buffer =  malloc(sizeof(double)*size);
    if(buffer->buffer == NULL){
        printf("Nothing inside buffer\n");
    }
        // handle error
    buffer->capacity = size;
    //buffer->count =0;
    circular_buf_reset(buffer);

    return buffer;
}

void circular_buf_free(circular_buf_t* cbuf) {
    free(cbuf->buffer);
}

void circular_buf_reset(circular_buf_t* cbuf){
    cbuf-> head =0;
    cbuf->tail =0;
    cbuf->full = false;
}
int circular_buf_getCount(circular_buf_t* cbuf){
    int count = cbuf->capacity; // if the array is full, return max
    if(!circular_buf_full(cbuf)){ //if the array is not full
        if(cbuf->head >= cbuf->tail){
            count = cbuf->head - cbuf->tail;
        }
        else
            count = cbuf->capacity+ (cbuf->head - cbuf->tail);

    }
    return count;
}

static inline int advance_headtail_value(int value, int max) 
{
	return (value + 1) % max;
}

static void advance_pointer(circular_buf_t* cbuf) // this function to advance the head and tail pointer
{
	if(cbuf->full)
   	{   
        //advance tail by 1 
        //reset tail to beginning if the buffer is full
		if (++(cbuf->tail) == cbuf->capacity) 
		{ 
			cbuf->tail = 0;
		}
	}
    //if the buffer is not full
    // advance head by 1
	if (++(cbuf->head) == cbuf->capacity) 
	{ 
		cbuf->head = 0;
	}
	cbuf->full = (cbuf->head == cbuf->tail); //return the state of buffer
}

static void retreat_pointer(circular_buf_t* me)
{
	//assert(me);

	me->full = false;
	if (++(me->tail) == me->capacity) 
	{ 
		me->tail = 0;
	}
}

void circular_buf_put(circular_buf_t* cbuf, double data){ //put data into the circular buffer
    cbuf->buffer[cbuf->head]= data; //insert data to the head position of the buffer
    advance_pointer(cbuf); //move the head and tail pointers accordingly
}

double circular_buf_get(circular_buf_t* me, double* data) //return the data
{
    //assert(me && data && me->buffer);

    int r = -1;

    if(!circular_buf_empty(me))
    {
        *data = me->buffer[me->tail];
        retreat_pointer(me);

        r = 0;
    }

    return r;
}

int circular_buf_capacity(circular_buf_t* cbuf){
    return cbuf->capacity;
}
// int circular_buf_getCount(circular_buf_t* cbuf){
//     return cbuf->count;
// }

void circular_buf_setCapacity(circular_buf_t* cbuf, int newSize){
    cbuf->capacity = newSize;
}


bool circular_buf_full(circular_buf_t* me)
{
	//assert(me);

	return me->full;
}

bool circular_buf_empty(circular_buf_t* me) //check if the array is empty
{
    return (!circular_buf_full(me) && (me->head == me->tail));
}
double* circular_buf_returnBuf(circular_buf_t* cbuf, int length){
    double *return_arr = (double*)malloc(sizeof(int) * length);
    for (int i = 0; i < length; i++) {
       return_arr[i] = cbuf->buffer[i];
    }

    return return_arr;

}