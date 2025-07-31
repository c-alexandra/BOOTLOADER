#pragma once

#include "common.h"

uint8_t crc8(uint8_t* data, const uint32_t length);

uint32_t crc32(const uint8_t* data, const uint32_t length);