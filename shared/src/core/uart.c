/*******************************************************************************
 * @file   uart.c
 * @author Camille Aitken
 *
 * @brief Implement simple UART communication
 ******************************************************************************/

// External library includes
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

// User includes
#include "common.h"
#include "core/uart.h"
#include "core/ring-buffer.h"

// Defines & macros
#define BAUD_RATE (115200)
#define RING_BUFFER_SIZE (uint32_t)(256) // must be a power of 2


// Global and Extern Declarations
static uint8_t data_buffer[RING_BUFFER_SIZE] = {0U};
static ring_buffer_t rb = {0U};

// Functions

/**
 * @brief USART1 interrupt service routine to write to ring buffer
 */
void usart1_isr(void) {
    const bool overrun_occurred = usart_get_flag(USART1, USART_FLAG_ORE) == 1;
    const bool received_data = usart_get_flag(USART1, USART_FLAG_RXNE) == 1;

    // when uart receives data, write a byte to the ring buffer
    if (received_data || overrun_occurred) {
        if(!ring_buffer_write(&rb, (uint8_t)usart_recv(USART1))) {
            // buffer full, do something, handle failure
        }
    }
}

/** 
 * @brief initialize interal configuration for enabling UART on STM32F446RE
 * 
 * @note This function sets up the UART peripheral on the STM32F446RE. It also 
 * initializes the ring buffer for storing incoming data.
 */
void uart_setup(void) {
    rcc_periph_clock_enable(RCC_USART1);

    // older computers used many more control signals to dictate how lines 
    // should be received, and how serial lines are defined. 
    // We will ignore additional signals, so set none
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    usart_set_databits(USART1, 8);
    usart_set_baudrate(USART1, BAUD_RATE);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_stopbits(USART1, 1);

    // want to receive and transmit
    usart_set_mode(USART1, USART_MODE_TX_RX);

    /**
     * dma (direct memory access) allows you to write the received uart data 
     * directly into a space in memory, rather than needing to move it into a 
     * buffer. Allows writing to memory/registers where you would need to write
     * code to handle the data instead
     */
    // usart_enable_rx_dma(USART1);

    usart_enable_rx_interrupt(USART1);
    nvic_enable_irq(NVIC_USART1_IRQ);

    usart_enable(USART1);

    // initialize ring buffer
    ring_buffer_setup(&rb, data_buffer, RING_BUFFER_SIZE);
}

/** 
 * @brief Write data out to the UART buffer
 * @param data Pointer to the data structure to write
 * @param length The number of bytes to write
 */
void uart_send(uint8_t* data, const uint32_t length){
    for (uint32_t i = 0; i < length; ++i) {
        uart_send_byte(data[i]);
    }
}

/**
 * @brief Write a single byte out from USART1_TX
 * @param data The byte to write
 */
void uart_send_byte(uint8_t data) {
    usart_send_blocking(USART1, (uint16_t)data);
}

/** 
 * @brief Read data from the UART buffer
 * @param data Pointer to the data structure to read into
 * @param length The number of bytes to read
 * @return The number of bytes read
 */
uint32_t uart_receive(uint8_t* data, const uint32_t length) {
    // attempt to read from the ring buffer
    if (length > 0) {
        for (uint32_t bytes_read = 0; bytes_read < length; ++bytes_read) {
            // if we can't fully read from the buffer, return the number of bytes read
            if(!ring_buffer_read(&rb, &data[bytes_read])) {
                return bytes_read;
            }
        }
    }

    return length;
}

/**
 * @brief Read a single byte from the UART buffer
 * @return The byte read
 */
uint8_t uart_receive_byte(void) {
    uint8_t byte = 0;
    
    (void)uart_receive(&byte, 1);

    return byte;
}

/**
 * @brief Check if data is available in the UART ring buffer
 * @return True if data is available, False otherwise
 */
bool uart_data_available(void) {
    return !ring_buffer_empty(&rb);
}
