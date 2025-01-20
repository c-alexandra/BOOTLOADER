/*******************************************************************************
 * @file   bootloader.c
 * @author Camille Alexandra
 *
 * @brief  Redirect vector table to launch with custom bootloader
 ******************************************************************************/

// External library includes
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/stm32/rcc.h>
// #include <libopencm3/cm3/vector.h> // if using libopencm3 vector table

// User includes
#include "common.h"
#include "core/uart.h"
#include "core/system.h"
#include "comms.h"

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

    /**
     * The following implementation uses the libopencm3 vector table
     * and uses the libopencm3/cm3/vector.h file to call the reset function
     */ 
    // vector_table_t* vector_table = (vector_table_t*)MAIN_APP_START_ADDRESS;
    // main_vector_table->reset();
}

// breaks linker if rom set to 32kb
// const uint8_t data[0x8000] = {0};
// static void test_rom_size(void) {
//     volatile uint8_t x = 0;
//     for (uint32_t i = 0; i < 0x8000; ++i) {
//         x += data[i];
//     }
// }

static void gpio_setup(void) {
    // enable rcc for GPIOA
    rcc_periph_clock_enable(RCC_GPIOA);

    //configure uart
    gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TX_PIN | RX_PIN);
    gpio_set_af(UART_PORT, GPIO_AF7, TX_PIN | RX_PIN);
}


int main(void) {
    system_setup();
    gpio_setup();
    uart_setup();
    comms_setup();

    // while (true) {
    //     system_delay(1000);
    // }

    // test_rom_size(); // DEBUG: breaks linker if rom set to 32kb

    jump_to_main();

    return 0;
}