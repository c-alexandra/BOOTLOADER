/*******************************************************************************
 * @file   ring-buffer.c
 * @author Camille Aitken
 *
 * @brief  Implements the ring buffer data structure
 ******************************************************************************/

#include "core/ring-buffer.h"

/*******************************************************************************
 * @brief Setup the ring buffer object
 * 
 * @param rb Pointer to the ring buffer object
 * @param data Pointer to the data buffer
 * @param size Size of the data buffer, as a power of 2
 ******************************************************************************/
void ring_buffer_setup(ring_buffer_t* rb, uint8_t* buffer, uint32_t size) {
    rb->buffer = buffer;
    rb->mask = size - 1; // used for bitwise AND to wrap around buffer
    rb->head = 0;
    rb->tail = 0;
}

/*******************************************************************************
 * @brief Check if the ring buffer is empty
 * 
 * @param rb Pointer to the ring buffer object
 * @return True if the buffer is empty, False otherwise
 ******************************************************************************/
bool ring_buffer_empty(ring_buffer_t* rb) {
    return (rb->head == rb->tail);
}

/*******************************************************************************
 * @brief Check if the ring buffer is full
 * 
 * @param rb Pointer to the ring buffer object
 * @param data Data to write to the buffer
 * @return True if the write was successful, False otherwise
 ******************************************************************************/
bool ring_buffer_write(ring_buffer_t* rb, uint8_t data) {
    // make local copy to safeguard concurrent rb accesses
    uint32_t local_read_index = rb->head;
    uint32_t local_write_index = rb->tail;

    // Check if buffer is completely full, ie tail is right before head
    // TODO: This is a bug, the buffer is full and something needs to be done with the data - maybe flush it?
    if (local_read_index == ((local_write_index + 1) & rb->mask)) {
        return false;
    }

    // Write data and increment tail
    rb->buffer[local_write_index] = data;
    local_write_index = (local_write_index + 1) & rb->mask;
    rb->tail = local_write_index;

    return true;
}

/******************************************************************************* 
 * @brief Read a byte from the ring buffer
 * 
 * @param rb Pointer to the ring buffer object
 * @param data Pointer to the data to read into
 * @return True if the read was successful, False otherwise
 ******************************************************************************/
bool ring_buffer_read(ring_buffer_t* rb, uint8_t* data) {
    // make local copy to safeguard concurrent rb accesses
    uint32_t local_read_index = rb->head;
    uint32_t local_write_index = rb->tail;

    // Check if buffer is empty
    if (local_read_index == local_write_index) {
        return false;
    }

    // Write data and increment tail
    *data = rb->buffer[local_read_index];
    local_read_index = (local_read_index + 1) & rb->mask;
    rb->head = local_read_index;

    return true;
}
