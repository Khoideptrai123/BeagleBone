#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include"circularbuffer.h"

struct circular_buf_t{
    double* buffer;
	int head ;
	int tail;
	int capacity; // of the buffer
    //int count; // count the number of elements in the buffer
	//bool full;

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
    //cbuf->full = false;
}
int circular_buf_getCount(circular_buf_t* cbuf){ // return the current size of the circular buffer
    int count = cbuf->capacity; // if the array is full, return max
    if(circular_buf_empty(cbuf)){ // if the array is empty, return 0
        return 0;
    }
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
	if(circular_buf_full(cbuf))
   	{   
        //advance tail by 1 
        //reset tail to beginning when the buffer is full
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
	//cbuf->full = (cbuf->head == cbuf->tail); //return the state of buffer
}

static void retreat_pointer(circular_buf_t* me)
{
	//assert(me);

	//me->full = false;
	if (++(me->tail) == me->capacity) 
	{ 
		me->tail = 0;
	}
}

/// For thread safety, do not use put - use try_put.
/// Because this version, which will overwrite the existing contents
/// of the buffer, will involve modifying the tail pointer, which is also
/// modified by get.
void circular_buf_put(circular_buf_t* cbuf, double data){ //put data into the circular buffer
    advance_pointer(cbuf); //move the head and tail pointers accordingly
    cbuf->buffer[cbuf->head]= data; //insert data to the head position of the buffer
    
}

int circular_buf_try_put(circular_buf_t* me, double data)
{
    int r = -1;
	if(circular_buf_empty(me)){
        (me->buffer)[0] = data;
        r =1;
    }
	
    /* When the buffer is full, */
	if(!circular_buf_full(me))
	{
		
		//me->head = advance_headtail_value(me->head, me->capacity);
        //advance_pointer(me);
        me->buffer[me->head] = data;
        advance_pointer(me);
		r = 0;
	}
    else {
        // if buffer is shift start and end index to the right
        // if one of the indexes are at the last location, set to 0
        me->tail = (me->tail)+1 >= me->capacity ? 0: me->tail +1; 
        me->head = (me->head)+1 >= me->capacity ? 0: me->head +1; 
        me->buffer[me->head] = data;
    }

	return r;
    // int next = me->head +1; // next is where head will point after this write
    // if (next >= me->capacity)
    //     next =0;
    // if(circular_buf_full(me)){
    //     me->tail = (me->tail)+1 >= me->capacity ? 0: me->tail +1; 
    //     me->head = (me->head)+1 >= me->capacity ? 0: me->head +1; 
    //     me->buffer[me->tail] = data;
    // }
    // me->buffer[me->head] = data;
    // me->head = next;
    // return 0;
}

double circular_buf_get(circular_buf_t* me, double* data) //return the data
{
    //assert(me && data && me->buffer);

    int r = -1;

    if(!circular_buf_empty(me))
    {
        *data = me->buffer[me->tail];
        //retreat_pointer(me);
        me->tail = advance_headtail_value(me->tail, me-> capacity);
        r = 0;
    }

    return r;
}

int circular_buf_capacity(circular_buf_t* cbuf){
    return cbuf->capacity;
}

void circular_buf_setCapacity(circular_buf_t* cbuf, int newSize){
    cbuf->capacity = newSize;
}


bool circular_buf_full(circular_buf_t* me)
{

	// We want to check, not advance, so we don't save the output here
	return advance_headtail_value(me->head, me->capacity) == me->tail;
}

bool circular_buf_empty(circular_buf_t* me) //check if the array is empty
{
    return (!circular_buf_full(me) && (me->head == me->tail));
}

/* Copy array of items in the Circular buffer from oldest to newest*/
double* circular_buf_returnBuf(circular_buf_t* cbuf, double *arr){
    int counter =0;
    // if buffer empty do nothing
    if( circular_buf_empty(cbuf)){
        return arr;
    }

    // head >= tail, just look from start to end
    if(cbuf->tail <= cbuf->head){
        //printf("Case 1 \n");
        for(int i = cbuf->tail; i <= cbuf->head; i++){
            //printf("DITMEMAY: %d\n",counter);
            arr[counter]= cbuf->buffer[i];
            counter++;
        }
    }
    else{ 
        //printf("Case 2\n");
             
        // Loop from start to end of array (head < tail), array is full
        //Buffer is full and end < start
		//                 end start         
		//                  |  | 
		// ex. [0][1][2][3][4][5][6][7]

        // First, it fills the array from start(5) to capacity (7)
       for (int i = cbuf -> tail; i < cbuf->capacity; i++) {
            arr[counter] = cbuf->buffer[i];
            counter++;
        }

        // Then loop from 0 to end index
        for (int i =0; i <= cbuf ->head; i++){
            arr[counter] = cbuf->buffer[i];
            counter++;
        }
        //printf("Case 2 Done \n");
    }
    return arr;   
}
void resizeBuffer(circular_buf_t *cbuf, int newSize){
    // Do nothing if there is no change in the buffer size
    if(circular_buf_capacity(cbuf) == newSize)
        return;
    
    // if the circular buffer is empty, reinitialize buffer with newSize
    if(circular_buf_empty(cbuf)){
        free(cbuf ->buffer);
        cbuf-> buffer = malloc(newSize*sizeof(double));
        cbuf-> head = 0;
        cbuf->tail = 0;
        cbuf-> capacity = newSize;
        return;
    }

    // Else, get a copy of the current buffer in order
    int numElement = circular_buf_getCount(cbuf);
    double currentArr[numElement];
    circular_buf_returnBuf(cbuf, currentArr);

    // De-allocating current array and relocate with new size
    free(cbuf->buffer);
    cbuf -> buffer = malloc(newSize*sizeof(double));
    //cbuf->head =0;
    //cbuf->tail =0;

    // When new size < current number of elements in array
    // only copy from circular_buf_getCount(cbuf) - new Size to current size

    if( newSize < numElement){
        //int sizeDiff =  circular_buf_getCount(cbuf) - newSize;
        // for(i = sizeDiff;i < circular_buf_getCount(cbuf); i++){
        //     cbuf -> buffer[i- sizeDiff] = currentArr[i];
        //     //circular_buf_try_put(cbuf, currentArr[i]);
        // }
        // cbuf->head = i - sizeDiff -1;
        for(int i =0; i < newSize; i++){
            cbuf->buffer[i] = currentArr[numElement-1-i];
        }
        cbuf->head = newSize-1;
        cbuf->tail =0;

    }
    else{
        // when new size > current number of elements => copy everything
        for(int i =0; i < circular_buf_getCount(cbuf); i++){
            cbuf->buffer[i] = currentArr[i];
            //circular_buf_try_put(cbuf, currentArr[i]);
        }
        //cbuf->head = circular_buf_getCount(cbuf)-1;
    }

    // re-define parameters:
    cbuf->capacity = newSize;

} 

void displayBuffer(circular_buf_t *buffer, int skip){
    //printf("Display buffer \n");
    int currentSize  = circular_buf_getCount(buffer);
    double currentArr[currentSize];
    circular_buf_returnBuf(buffer, currentArr);
    printf("Sample in the history: \t");
    for(int i=0; i < currentSize; i+= skip){ //skip certain items based on arguments
        printf("%0.3f  ", currentArr[i]);
    }
    printf("\n");
}