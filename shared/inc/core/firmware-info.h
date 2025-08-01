#pragma once

#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/vector.h>
#include "common.h"


#define DEVICE_ID (0xA3) // arbitrary device id to identify for fw updates

#define BOOTLOADER_SIZE        (0x8000U) // 32KB - 32 768 bytes
#define FLASH_MEM_BEGIN        (0x08000000)
#define FLASH_MEM_BOOTLOADER   (0x08008000)
#define FLASH_MEM_END          (0x081FFFFF)
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FW_LENGTH          ((1024U * 512U) - BOOTLOADER_SIZE) // 512KB

// placed after vector table + account for byte alignment 
#define FWINFO_ADDRESS (MAIN_APP_START_ADDRESS + sizeof(vector_table_t) + sizeof(uint32_t) * 1)

#define FWINFO_VALIDATE_FROM   (FWINFO_ADDRESS + sizeof(firmware_info_t))
#define FWINFO_VALIDATE_LENGTH(fw_length) (fw_length - (sizeof(vector_table_t) + sizeof(firmware_info_t) + sizeof(uint32_t) * 1))
#define FWINFO_SENTINEL (0xDEADC0DE) // Example sentinel value to identify firmware info structure

// placed directly after the interrupt vector table in flash
// check datasheet for exact address and table size
typedef struct firmware_info_t {
    uint32_t sentinel;  // Sentinel value to identify firmware info structure
    uint32_t device_id; // Unique device identifier
    uint32_t version;   // Firmware version number
    uint32_t length;    // length of the firmware image
    uint32_t reserved0;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
    uint32_t crc32;     // unique crc32 checksum of the firmware image
} firmware_info_t;

bool validate_firmware_image(void);