#include <string.h>


#include "common.h"
#include "core/firmware-info.h"
#include "core/crc.h"
#include "core/aes.h"

// TODO: implement a proper secret key for AES encryption
static const uint8_t secret_key[AES_BLOCK_SIZE] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
};

static void aes_cbc_mac_step(AES_Block_t state, AES_Block_t prev_state, AES_Block_t key) {
    // cbc chaining operation
    for (uint8_t i = 0; i < AES_BLOCK_SIZE; ++i) {
        ((uint8_t*)state)[i] ^= ((uint8_t*)prev_state)[i];
    }

    AES_EncryptBlock(state, &key);
    memcpy(prev_state, state, AES_BLOCK_SIZE);
}

/*******************************************************************************
 * @brief Validate the firmware image by checking the sentinel value and CRC
 * 
 * @return True if the firmware image is valid, False otherwise
 ******************************************************************************/
bool validate_firmware_image(void) {
    // point to firmware metadata in provided firmware image
    firmware_info_t* info = (firmware_info_t *)FWINFO_ADDRESS;
    const uint8_t* signature = (const uint8_t*)FW_SIGNATURE_ADDRESS;

    // Check sentinel value
    if (info->sentinel != FWINFO_SENTINEL) {
        return false;
    }
    // Check device ID
    if (info->device_id != DEVICE_ID) {
        return false;
    }

    /** aes signature validation
     * - block by block, use aes-cbc (cbc_mac) encr process
     * - start encrypt the firmware info section
     * - ignore signature section
     * - after final block, memcmp the signature with what is stored
     */
    AES_Block_t round_keys[NUM_ROUND_KEYS_128];
    AES_KeySchedule128(secret_key, round_keys);

    AES_Block_t state = {};
    AES_Block_t prev_state = {};

    // generic padding handler to ensure proper AES block size
    uint8_t bytes_to_pad = 16 - info->length % AES_BLOCK_SIZE;
    if (bytes_to_pad == 0) {
        bytes_to_pad = AES_BLOCK_SIZE;
    }

    // copy firmware info section manually first
    uint32_t fwinfo_offset = 0;
    while (fwinfo_offset < FWINFO_BLOCK_SIZE) {
        memcpy(state, info + fwinfo_offset, AES_BLOCK_SIZE);
        aes_cbc_mac_step(state, prev_state, round_keys);
        fwinfo_offset + AES_BLOCK_SIZE;
    }

    uint32_t offset = 0;
    while (offset < info->length) {
        // skip firmware info and signature block
        if (offset == (FWINFO_ADDRESS - MAIN_APP_START_ADDRESS)) {
            offset += AES_BLOCK_SIZE + FWINFO_BLOCK_SIZE;
            continue;
        }

        if (info->length - offset > AES_BLOCK_SIZE) { // normal case
            // normal case
            memcpy(state, (void*)(MAIN_APP_START_ADDRESS + offset), AES_BLOCK_SIZE);
            aes_cbc_mac_step(state, prev_state, round_keys);
        } else { // padding case
            // add whole extra block of passing
            if (bytes_to_pad == 16) {
                memcpy(state, (void*)(MAIN_APP_START_ADDRESS + offset), AES_BLOCK_SIZE);
                aes_cbc_mac_step(state, prev_state, round_keys);

                memset(state, AES_BLOCK_SIZE, AES_BLOCK_SIZE);
                aes_cbc_mac_step(state, prev_state, round_keys);
            } else { // add padding in-block
                memcpy(state, (void*)(MAIN_APP_START_ADDRESS + offset), AES_BLOCK_SIZE - bytes_to_pad);
                memset((void*)state + (AES_BLOCK_SIZE - bytes_to_pad), bytes_to_pad, bytes_to_pad);
                aes_cbc_mac_step(state, prev_state, round_keys);
            }
        }
        offset += AES_BLOCK_SIZE;
    }

    return memcmp(signature, state, AES_BLOCK_SIZE);

    return true;
}