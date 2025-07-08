#pragma once

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/memorymap.h> // FLASH_BASE

#include "common.h"

#define BOOTLOADER_SIZE        (0x8000U) // 32KB - 32 768 bytes
#define FLASH_MEM_BEGIN        (0x08000000)
#define FLASH_MEM_BOOTLOADER   (0x08008000)
#define FLASH_MEM_END          (0x081FFFFF)
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FW_LENGTH          ((1024U * 512U) - BOOTLOADER_SIZE) // 512KB

#define CPU_FREQ         (84000000)
#define SYSTICK_FREQ     (1000)

void system_setup(void);
uint64_t system_get_ticks(void);

void system_delay(uint64_t milliseconds);
void system_teardown(void);