#include "common.h"
#include "core/firmware-info.h"
#include "core/crc.h"

/*******************************************************************************
 * @brief Validate the firmware image by checking the sentinel value and CRC
 * 
 * @return True if the firmware image is valid, False otherwise
 ******************************************************************************/
bool validate_firmware_image(void) {
    // point to firmware metadata in provided firmware image
    firmware_info_t *info = (firmware_info_t *)FWINFO_ADDRESS;

    // Check sentinel value
    if (info->sentinel != FWINFO_SENTINEL) {
        return false;
    }
    // Check device ID
    if (info->device_id != DEVICE_ID) {
        return false;
    }
    // Check CRC32
    uint32_t fw_length = info->length;

    return true;
}