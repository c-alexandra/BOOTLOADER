#pragma once

#include "common.h"

void bl_flash_erase_main_app(void);
void bl_flash_write_main_app(const uint32_t address, const uint8_t* data, uint32_t length);
