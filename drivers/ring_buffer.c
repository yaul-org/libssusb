/*
 * Copyright (c) 2014 Anders Kalør
 * MIT License (MIT).
 */

#include <stdlib.h>

#include "ring_buffer.h"

/**
 * @file
 * Implementation of ring buffer functions.
 */

int
ring_buffer_init(ring_buffer_t *buffer, size_t size)
{
        if (buffer == NULL) {
                return -1;
        }

        if (size > 0) {
                buffer->buffer = malloc(size);
                if (buffer->buffer == NULL) {
                        return -2;
                }
        }

        buffer->buffer_size = size;

        buffer->tail_index = 0;
        buffer->head_index = 0;

        return 0;
}

void
ring_buffer_deinit(ring_buffer_t *buffer)
{
        if (buffer == NULL) {
                return;
        }

        free(buffer->buffer);

        buffer->buffer = NULL;
        buffer->buffer_size = 0;

        buffer->tail_index = 0;
        buffer->head_index = 0;
}

void
ring_buffer_queue(ring_buffer_t *buffer, uint8_t data)
{
        /* Is buffer full? */
        if(ring_buffer_full(buffer)) {
                /* Is going to overwrite the oldest byte */
                /* Increase tail index */
                buffer->tail_index = ((buffer->tail_index + 1) & ring_buffer_mask(buffer));
        }

        /* Place data in buffer */
        buffer->buffer[buffer->head_index] = data;
        buffer->head_index = ((buffer->head_index + 1) & ring_buffer_mask(buffer));
}

void
ring_buffer_array_queue(ring_buffer_t *buffer, const uint8_t *data, ring_buffer_size_t size)
{
        /* Add bytes; one by one */
        ring_buffer_size_t i;
        for(i = 0; i < size; i++) {
                ring_buffer_queue(buffer, data[i]);
        }
}

uint8_t
ring_buffer_dequeue(ring_buffer_t *buffer, uint8_t *data)
{
        if(ring_buffer_empty(buffer)) {
                /* No items */
                return 0;
        }

        *data = buffer->buffer[buffer->tail_index];
        buffer->tail_index = ((buffer->tail_index + 1) & ring_buffer_mask(buffer));
        return 1;
}

ring_buffer_size_t
ring_buffer_array_dequeue(ring_buffer_t *buffer, uint8_t *data, ring_buffer_size_t len)
{
        if(ring_buffer_empty(buffer)) {
                /* No items */
                return 0;
        }

        uint8_t *data_ptr = data;
        ring_buffer_size_t cnt = 0;
        while((cnt < len) && ring_buffer_dequeue(buffer, data_ptr)) {
                cnt++;
                data_ptr++;
        }
        return cnt;
}

uint8_t
ring_buffer_peek(ring_buffer_t *buffer, uint8_t *data, ring_buffer_size_t index)
{
        if(index >= ring_buffer_size(buffer)) {
                /* No items at index */
                return 0;
        }

        /* Add index to pointer */
        ring_buffer_size_t data_index = ((buffer->tail_index + index) & ring_buffer_mask(buffer));
        *data = buffer->buffer[data_index];
        return 1;
}

extern inline uint8_t ring_buffer_empty(ring_buffer_t *buffer);
extern inline uint8_t ring_buffer_full(ring_buffer_t *buffer);
extern inline ring_buffer_size_t ring_buffer_size(ring_buffer_t *buffer);
