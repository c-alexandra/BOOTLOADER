/*******************************************************************************
 * @file   comms.c
 * @author Camille Aitken
 *
 * @brief
 ******************************************************************************/

// External library includes

// User includes
#include "comms.h"
#include "core/uart.h"

// Defines & macros

// Global and Extern Declarations

// Functions

/**
 * @brief Setup the communication peripheral
 */
void comms_setup(void) {

}

/** 
 * @brief Update the communication peripheral
 */
void comms_update(void) {
}

/** 
 * @brief Check if data is available in the communication peripheral
 * @return True if data is available, False otherwise
 */
bool comms_data_available(void) {

}

/** 
 * @brief Send a packet of data
 * @param packet Pointer to the packet to send
 */
void comms_send_packet(comms_packet_t* packet) {

}

/**
 * @brief Receive a packet of data
 * @param packet Pointer to packet buffer to write into
 */
void comms_receive_packet(comms_packet_t* packet) {

}
