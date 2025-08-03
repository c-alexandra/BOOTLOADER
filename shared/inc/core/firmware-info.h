#pragma once

#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/vector.h>
#include "common.h"

#define ALIGNED(address, alignment) (((address) - 1U + (alignment)) & ~((alignment) - 1U))

#define BOOTLOADER_SIZE        (0x8000U) // 32KB - 32 768 bytes
// #define FLASH_MEM_BEGIN        (0x08000000)
// #define FLASH_MEM_BOOTLOADER   (0x08008000)
// #define FLASH_MEM_END          (0x081FFFFF)
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FW_LENGTH          ((1024U * 512U) - BOOTLOADER_SIZE) // 512KB
#define DEVICE_ID (0xA3) // arbitrary device id to identify for fw updates


#define FWINFO_SENTINEL        (0xDEADC0DE)
#define FWINFO_ADDRESS         (ALIGNED(MAIN_APP_START_ADDRESS + sizeof(vector_table_t), 16))
#define FWINFO_BLOCK_SIZE      (sizeof(firmware_info_t))
#define SIGNATURE_ADDRESS      (FWINFO_ADDRESS + sizeof(firmware_info_t))
// #define FWINFO_VALIDATE_FROM   (ALIGNED(FWINFO_ADDRESS + sizeof(firmware_info_t), 16))
// #define FWINFO_VALIDATE_LENGTH(fw_length) (fw_length - (BOOTLOADER_SIZE - FWINFO_VALIDATE_FROM))
// #define FWINFO_BLOCK_SIZE (16 * 2) // size of the firmware info block
// #define FW_SIGNATURE_ADDRESS (ALIGNED(FWINFO_ADDRESS + sizeof(firmware_info_t), 16))

// placed directly after the interrupt vector table in flash
// check datasheet for exact address and table size
typedef struct firmware_info_t {
    uint32_t sentinel;    // Sentinel value to identify firmware info structure
    uint32_t device_id;   // Unique device identifier
    uint32_t version;     // Firmware version number
    uint32_t length;      // length of the firmware image
    // uint32_t reserved[4]; 
} firmware_info_t;

bool validate_firmware_image(void);