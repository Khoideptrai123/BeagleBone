// Circular buffer initiation

#ifndef _CIRCULARBUFFER_H_
#define _CIRCULARBUFFER_H_


//define the circular buffer structure
typedef struct circular_buf_t circular_buf_t;

//Initiialization
circular_buf_t *circ_buf_init(int size);

// Free storage in the buffer
void circular_buf_free(circular_buf_t* cbuf);

// Reset the buffer to empty
void circular_buf_reset();

/// Put that continues to add data if the buffer is full
/// Old data is overwritten
/// Note: if you are using the threadsafe version, this API cannot be used, because
/// it modifies the tail pointer in some cases. Use circular_buf_try_put instead.
/// Requires: me is valid and created by circular_buf_init
void circular_buf_put(circular_buf_t* cbuf, double data);



/// Retrieve a value from the buffer
/// Requires: me is valid and created by circular_buf_init
/// Returns 0 on success, -1 if the buffer is empty
double circular_buf_get(circular_buf_t* cbuf, double* data);

/// CHecks if the buffer is empty
/// Requires: me is valid and created by circular_buf_init
/// Returns true if the buffer is empty
bool circular_buf_empty(circular_buf_t* cbuf);

/// Checks if the buffer is full
/// Requires: me is valid and created by circular_buf_init
/// Returns true if the buffer is full
bool circular_buf_full(circular_buf_t* cbuf);

/// Check the capacity of the buffer
/// Requires: me is valid and created by circular_buf_init
/// Returns the maximum capacity of the buffer
int circular_buf_capacity(circular_buf_t* cbuf);

/// Check the number of elements stored in the buffer
/// Requires: me is valid and created by circular_buf_init
/// Returns the current number of elements in the buffer
int circular_buf_getCount(circular_buf_t* cbuf);

/// Look ahead at values stored in the circular buffer without removing the data
/// Requires:
///		- me is valid and created by circular_buf_init
///		- look_ahead_counter is less than or equal to the value returned by circular_buf_size()
/// Returns 0 if successful, -1 if data is not available
// int circular_buf_peek(circular_buf_t cbuf, int* data, unsigned int look_ahead_counter);

// TODO: int circular_buf_get_range(circular_buf_t me, uint8_t *data, size_t len);
// TODO: int circular_buf_put_range(circular_buf_t me, uint8_t * data, size_t len);


void circular_buf_setCapacity(circular_buf_t* cbuf, int newSize);

double* circular_buf_returnBuf(circular_buf_t* cbuf, int length);




#endif