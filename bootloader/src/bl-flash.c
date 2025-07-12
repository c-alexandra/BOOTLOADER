/*******************************************************************************
 * @file   bl-flash.c
 * @author Camille Aitken
 *
 * @brief Contains implementation for flash memory operations in the bootloader
 *        such as erasing and writing the main application.
 ******************************************************************************/

// External library includes
#include <libopencm3/stm32/flash.h>

// User includes
#include "bl-flash.h"

// Defines & macros
#define BOOTLOADER_SIZE (0x8000U) // 32KB
#define MAIN_APP_SIZE   (0x80000U - BOOTLOADER_SIZE) // 

#define MAIN_APP_SECTOR_START  (2)
#define MAIN_APP_SECTOR_END    (7)

/*******************************************************************************
 * @brief Erase the main application flash memory
 * 
 * This function unlocks the flash memory, erases the sectors allocated for the
 * main application, and then locks the flash memory again.
 ******************************************************************************/
void bl_flash_erase_main_app(void) {
    // Erase the main application flash memory
    flash_unlock();

    for (uint8_t sector = MAIN_APP_SECTOR_START; sector <= MAIN_APP_SECTOR_END; ++sector) {
        flash_erase_sector(sector, FLASH_CR_PROGRAM_X32);
    }

    flash_lock();
}

/*******************************************************************************
 * @brief Write data to the main application flash memory
 * 
 * This function unlocks the flash memory, writes the specified data to the
 * given address, and then locks the flash memory again.
 * 
 * @param address The starting address in flash memory where data will be written
 * @param data Pointer to the data to be written
 * @param length The length of the data to be written in bytes
 ******************************************************************************/
void bl_flash_write_main_app(const uint32_t address, 
    const uint8_t* data, uint32_t length) {
    flash_unlock();
    
    flash_program(address, data, length);

    flash_lock();
}