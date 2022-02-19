/**
 * @file
 * Prototypes and structures for the ring buffer module.
 */

#ifndef _LIBSSUSB_RING_BUFFER_H
#define _LIBSSUSB_RING_BUFFER_H

#include <inttypes.h>
#include <stddef.h>

/**
 * The type which is used to hold the size and the indicies of the buffer. Must
 * be able to fit \c ring_buffer_size(_r).
 */
typedef uint8_t ring_buffer_size_t;

/**
 * Used as a modulo operator
 * as <tt> a % b = (a & (b − 1)) </tt>
 * where \c a is a positive index in the buffer and
 * \c b is the (power of two) size of the buffer.
 */
#define ring_buffer_mask(_r) ((_r)->buffer_size - 1)

/**
 * Simplifies the use of <tt>struct ring_buffer_t</tt>.
 */
typedef struct ring_buffer_t ring_buffer_t;

/**
 * Structure which holds a ring buffer. The buffer contains a buffer array as
 * well as metadata for the ring buffer.
 */
struct ring_buffer_t {
        /** Buffer memory. */
        uint8_t *buffer;
        /** Buffer size in bytes. */
        size_t buffer_size;
        /** Index of tail. */
        ring_buffer_size_t tail_index;
        /** Index of head. */
        ring_buffer_size_t head_index;
};

/**
 * Initializes the ring buffer pointed to by <em>buffer</em>. This function can
 * also be used to empty/reset the buffer.
 *
 * @param buffer The ring buffer to initialize.
 */
void ring_buffer_init(ring_buffer_t *buffer, size_t size);

/**
 * Deinitializes the ring buffer point to by <em>buffer</em>.
 */
void ring_buffer_deinit(ring_buffer_t *buffer);

/**
 * Adds a byte to a ring buffer.
 * @param buffer The buffer in which the data should be placed.
 * @param data The byte to place.
 */
void ring_buffer_queue(ring_buffer_t *buffer, unsigned char data);

/**
 * Adds an array of bytes to a ring buffer.
 * @param buffer The buffer in which the data should be placed.
 * @param data A pointer to the array of bytes to place in the queue.
 * @param size The size of the array.
 */
void ring_buffer_queue_arr(ring_buffer_t *buffer, const unsigned char *data, ring_buffer_size_t size);

/**
 * Returns the oldest byte in a ring buffer.
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @return 1 if data was returned; 0 otherwise.
 */
uint8_t ring_buffer_dequeue(ring_buffer_t *buffer, unsigned char *data);

/**
 * Returns the <em>len</em> oldest bytes in a ring buffer.
 *
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the array at which the data should be placed.
 * @param len The maximum number of bytes to return.
 *
 * @return The number of bytes returned.
 */
ring_buffer_size_t ring_buffer_dequeue_arr(ring_buffer_t *buffer, unsigned char *data, ring_buffer_size_t len);

/**
 * Peeks a ring buffer, i.e. returns an element without removing it.
 *
 * @param buffer The buffer from which the data should be returned.
 * @param data A pointer to the location at which the data should be placed.
 * @param index The index to peek.
 *
 * @return 1 if data was returned; 0 otherwise.
 */
uint8_t ring_buffer_peek(ring_buffer_t *buffer, unsigned char *data, ring_buffer_size_t index);

/**
 * Returns whether a ring buffer is empty.
 *
 * @param buffer The buffer for which it should be returned whether it is empty.
 *
 * @return 1 if empty; 0 otherwise.
 */
inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer) {
        return (buffer->head_index == buffer->tail_index);
}

/**
 * Returns whether a ring buffer is full.
 * @param buffer The buffer for which it should be returned whether it is full.
 * @return 1 if full; 0 otherwise.
 */
inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer) {
        return ((buffer->head_index - buffer->tail_index) & ring_buffer_mask(buffer)) == ring_buffer_mask(buffer);
}

/**
 * Returns the number of items in a ring buffer.
 * @param buffer The buffer for which the number of items should be returned.
 * @return The number of items in the ring buffer.
 */
inline ring_buffer_size_t ring_buffer_count(ring_buffer_t *buffer) {
        return ((buffer->head_index - buffer->tail_index) & ring_buffer_mask(buffer));
}

#endif /* _LIBSSUSB_RING_BUFFER_H */
