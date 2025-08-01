#include <string.h>

#include "common.h"
#include "core/firmware-info.h"
#include "core/aes.h"

static const uint8_t secret_key[AES_BLOCK_SIZE] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0e, 0x0f,
};

static void aes_cbc_mac_step(AES_Block_t aes_state, AES_Block_t prev_state, const AES_Block_t *key_schedule) {
  // The CBC chaining operation
  for (uint8_t i = 0; i < AES_BLOCK_SIZE; i++) {
    ((uint8_t*)aes_state)[i] ^= ((uint8_t*)prev_state)[i];
  }

  AES_EncryptBlock(aes_state, key_schedule);
  memcpy(prev_state, aes_state, AES_BLOCK_SIZE);
}

static bool validate_firmware_image(void) {
  firmware_info_t* firmware_info = (firmware_info_t*)FWINFO_ADDRESS;
  const uint8_t* signature = (const uint8_t*)SIGNATURE_ADDRESS;

  if (firmware_info->sentinel != FWINFO_SENTINEL) {
    return false;
  }

  if (firmware_info->device_id != DEVICE_ID) {
    return false;
  }

  AES_Block_t round_keys[NUM_ROUND_KEYS_128];
  AES_KeySchedule128(secret_key, round_keys);

  AES_Block_t aes_state = {0};
  AES_Block_t prev_state = {0};

  uint8_t bytes_to_pad = 16 - (firmware_info->length % 16);
  if (bytes_to_pad == 0) {
    bytes_to_pad = 16;
  }

  memcpy(aes_state, firmware_info, AES_BLOCK_SIZE);
  aes_cbc_mac_step(aes_state, prev_state, round_keys);

  uint32_t offset = 0;
  while (offset < firmware_info->length) {
    // Are we are the point where we need to skip the info and signature sections?
    if (offset == (FWINFO_ADDRESS - MAIN_APP_START_ADDRESS)) {
      offset += AES_BLOCK_SIZE * 2;
      continue;
    }

    if (firmware_info->length - offset > AES_BLOCK_SIZE) {
      // The regular case
      memcpy(aes_state, (void*)(MAIN_APP_START_ADDRESS + offset), AES_BLOCK_SIZE);
      aes_cbc_mac_step(aes_state, prev_state, round_keys);
    } else {
      // The case of padding
      if (bytes_to_pad == 16) {
        // Add a whole extra block of padding
        memcpy(aes_state, (void*)(MAIN_APP_START_ADDRESS + offset), AES_BLOCK_SIZE);
        aes_cbc_mac_step(aes_state, prev_state, round_keys);

        memset(aes_state, AES_BLOCK_SIZE, AES_BLOCK_SIZE);
        aes_cbc_mac_step(aes_state, prev_state, round_keys);
      } else {
        memcpy(aes_state, (void*)(MAIN_APP_START_ADDRESS + offset), AES_BLOCK_SIZE - bytes_to_pad);
        memset((void*)(aes_state) + (AES_BLOCK_SIZE - bytes_to_pad), bytes_to_pad, bytes_to_pad);
        aes_cbc_mac_step(aes_state, prev_state, round_keys);
      }
    }

    offset += AES_BLOCK_SIZE;
  }

  return memcmp(signature, aes_state, AES_BLOCK_SIZE) == 0;
}
