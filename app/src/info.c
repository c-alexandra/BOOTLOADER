#include "core/firmware-info.h"

// prevents linker from optimizing out the firmware_info_t structure
__attribute__((section (".firmware_info")))
firmware_info_t firmware_info = {
    .sentinel    = FWINFO_SENTINEL,
    .device_id   = DEVICE_ID,
    .version     = 0xFFFFFFFF, 
    .length      = 0xFFFFFFFF, 
    .reserved[0] = 0x796e6974,
    .reserved[1] = 0x62756C20,
    .reserved[2] = 0xFFFFFF73,
    .reserved[3] = 0xFFFFFFFF,
};
