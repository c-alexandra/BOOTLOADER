/*******************************************************************************
 * @file   crc.c
 * @author Camille Aitken
 *
 * @brief Implement a simple CRC algorithm for verifying data integrity in 
 *        communication packets.
 ******************************************************************************/

#include "core/crc.h"

volatile int x = 0;

/*******************************************************************************
 * @brief Calculate the CRC-8 of a data buffer
 * 
 * @param data Pointer to the data buffer
 * @param length The number of bytes in the data buffer
 * @return The CRC-8 of the data buffer
 ******************************************************************************/
uint8_t crc8(uint8_t* data, const uint32_t length) {
    uint8_t crc = 0;

    for (uint32_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
        ++x;
    }

    return crc;
}

/*******************************************************************************
 * @brief Calculate the CRC-32 of a data buffer
 * 
 * @param data Pointer to the data buffer
 * @param length The number of bytes in the data buffer
 * @return The CRC-32 of the data buffer
 ******************************************************************************/
uint32_t crc32(const uint8_t* data, const uint32_t length) {
    uint8_t byte;
    uint32_t crc = 0xFFFFFFFF;
    uint32_t mask;

    for (uint32_t i = 0; i < length; ++i) {
        byte = data[i];
        crc ^= byte;

        for (uint8_t j = 0; j < 8; ++j) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return ~crc;
}