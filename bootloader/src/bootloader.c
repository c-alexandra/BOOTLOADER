/*******************************************************************************
 * @file   bootloader.c
 * @author Camille Alexandra
 *
 * @brief  Redirect vector table to launch with custom bootloader
 ******************************************************************************/

// External library includes
#include <libopencm3/stm32/memorymap.h>

// User includes
#include "../../shared/inc/common.h"

// Defines & Macros
#define BOOTLOADER_SIZE        (0x8000U) // 32 768
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)

// Global and Extern Declarations

// Functions
static void jump_to_main(void) {
    // create typedef for a void function
    typedef void (*void_fn)(void);

    // reset vector is second entry in table, stack pointer is first, thus + 4
    uint32_t* reset_vector_entry = (uint32_t*)(MAIN_APP_START_ADDRESS + 4U);
    uint32_t* reset_vector = (uint32_t*)(*reset_vector_entry);

    // interpret reset_vector as void function and call function
    void_fn reset_fnc = (void_fn)reset_vector;

    reset_fnc();
}

// breaks linker if rom set to 32kb
// const uint8_t data[0x8000] = {0};
// static void test_rom_size(void) {
//     volatile uint8_t x = 0;
//     for (uint32_t i = 0; i < 0x8000; ++i) {
//         x += data[i];
//     }
// }

int main(void) {
    // test_rom_size();

    jump_to_main();

    return 0;
}