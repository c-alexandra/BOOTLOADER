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

// Defines & macros
#define BAUD_RATE (115200)

// Global and Extern Declarations
static uint8_t data_buffer = 0U;
static bool data_available = false;

// Functions

void usart1_isr(void) {
    const bool overrun_occurred = usart_get_flag(USART1, USART_FLAG_ORE) == 1;
    const bool received_data = usart_get_flag(USART1, USART_FLAG_RXNE) == 1;

    if (received_data || overrun_occurred) {
        data_buffer = (uint8_t)usart_recv(USART1);
        data_available = true;
    }
}

/** @brief initialize interal configuration for enabling UART on STM32F446RE
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
}

/** @brief
 * 
 * @param data
 * @param length
 */
void uart_write(uint8_t* data, const uint32_t length){
    for (uint32_t i = 0; i < length; ++i) {
        uart_write_byte(data[i]);
    }
}

void uart_write_byte(uint8_t data) {
    usart_send_blocking(USART1, (uint16_t)data);
}

uint32_t uart_read(uint8_t* data, const uint32_t length) {
    if (length > 0 && data_available) {
        *data = data_buffer;
        data_available = false;
        return 1;
    }

    return 0;
}

uint8_t uart_read_byte(void) {
    data_available = false;
    return data_buffer;
}

bool uart_data_available(void) {
    return data_available;
}
