/*
 * The MIT License (MIT)
 * Copyright (c) 2014 Anders Kalør
 */

#include <stdlib.h>

#include "ringbuffer.h"

/**
 * @file
 * Implementation of ring buffer functions.
 */

void
ring_buffer_init(ring_buffer_t *buffer, size_t size)
{
        if (buffer == NULL) {
                return;
        }

        if (size > 0) {
                buffer->buffer = malloc(size);
        }

        buffer->buffer_size = size;

        buffer->tail_index = 0;
        buffer->head_index = 0;
}

void
ring_buffer_deinit(ring_buffer_t *buffer)
{
}

void
ring_buffer_queue(ring_buffer_t *buffer, char data)
{
        /* Is buffer full? */
        if(ring_buffer_is_full(buffer)) {
                /* Is going to overwrite the oldest byte */
                /* Increase tail index */
                buffer->tail_index = ((buffer->tail_index + 1) & ring_buffer_mask(buffer));
        }

        /* Place data in buffer */
        buffer->buffer[buffer->head_index] = data;
        buffer->head_index = ((buffer->head_index + 1) & ring_buffer_mask(buffer));
}

void
ring_buffer_queue_arr(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size)
{
        /* Add bytes; one by one */
        ring_buffer_size_t i;
        for(i = 0; i < size; i++) {
                ring_buffer_queue(buffer, data[i]);
        }
}

uint8_t
ring_buffer_dequeue(ring_buffer_t *buffer, char *data)
{
        if(ring_buffer_is_empty(buffer)) {
                /* No items */
                return 0;
        }

        *data = buffer->buffer[buffer->tail_index];
        buffer->tail_index = ((buffer->tail_index + 1) & ring_buffer_mask(buffer));
        return 1;
}

ring_buffer_size_t
ring_buffer_dequeue_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len)
{
        if(ring_buffer_is_empty(buffer)) {
                /* No items */
                return 0;
        }

        char *data_ptr = data;
        ring_buffer_size_t cnt = 0;
        while((cnt < len) && ring_buffer_dequeue(buffer, data_ptr)) {
                cnt++;
                data_ptr++;
        }
        return cnt;
}

uint8_t
ring_buffer_peek(ring_buffer_t *buffer, char *data, ring_buffer_size_t index)
{
        if(index >= ring_buffer_num_items(buffer)) {
                /* No items at index */
                return 0;
        }

        /* Add index to pointer */
        ring_buffer_size_t data_index = ((buffer->tail_index + index) & ring_buffer_mask(buffer));
        *data = buffer->buffer[data_index];
        return 1;
}

extern inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer);
extern inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer);
extern inline ring_buffer_size_t ring_buffer_num_items(ring_buffer_t *buffer);
