/*
 * circular_buffer.h
 *
 *  Created on: Jul 2, 2024
 *      Author: ASUS
 */

#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include <stdint.h>

#define UART2_BUFFER_SIZE 4096

typedef struct {
    uint8_t buffer[UART2_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} RingBuffer;



extern void ring_buffer_init(RingBuffer *rb);
int ring_buffer_write(RingBuffer *rb, uint8_t data);
int ring_buffer_read(RingBuffer *rb, uint8_t *data);


#endif /* INC_RING_BUFFER_H_ */
