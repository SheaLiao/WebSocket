/*
 * circular_buffer.c
 *
 *  Created on: Jul 2, 2024
 *      Author: ASUS
 */

#include <ringbuf.h>

void ring_buffer_init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

int ring_buffer_write(RingBuffer *rb, uint8_t data)
{
    if (rb->count >= UART2_BUFFER_SIZE)
    {
        // Buffer is full
        return -1;
    }

    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % UART2_BUFFER_SIZE;
    rb->count++;
    return 0;
}

int ring_buffer_read(RingBuffer *rb, uint8_t *data)
{
    if (rb->count == 0)
    {
        // Buffer is empty
        return -1;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % UART2_BUFFER_SIZE;
    rb->count--;
    return 0;
}

