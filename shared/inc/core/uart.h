#pragma once

#include "common.h"

void uart_setup(void);
void uart_teardown(void);
void uart_send(uint8_t* data, const uint32_t length);
void uart_send_byte(uint8_t data);
uint32_t uart_receive(uint8_t* data, const uint32_t length);
uint8_t uart_receive_byte(void);
bool uart_data_available(void);