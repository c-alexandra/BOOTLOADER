#include "core/firmware-info.h"

// prevents linker from optimizing out the firmware_info_t structure
__attribute__((section (".firmware_info")))
firmware_info_t firmware_info = {
    .sentinel   = FWINFO_SENTINEL,
    .device_id  = DEVICE_ID,
    .version    = 0xFFFFFFFF, 
    .length     = 0xFFFFFFFF, 
    .reserved0  = 0x796e6974,
    .reserved1  = 0x62756C20,
    .reserved2  = 0xFFFFFF73,
    .reserved3  = 0xFFFFFFFF,
    .reserved4  = 0xFFFFFFFF,
    .crc32      = 0xFFFFFFFF 
};
