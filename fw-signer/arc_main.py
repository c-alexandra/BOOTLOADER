#!/usr/bin/env python3

import sys
import os
import subprocess
import struct

BOOTLOADER_SIZE        = 0x8000  # 32KB
VECTOR_TABLE_SIZE      = 0x01B0  # 432 bytes
MAIN_APP_START_ADDRESS = 0x08008000  # Example start address for main application
MAX_FW_LENGTH          = (1024 * 512) - BOOTLOADER_SIZE  # 512KB - 32KB

AES_BLOCK_SIZE        = 16  # AES block size in bytes
SIGNATURE_SIZE        = 16  # Size of the signature in bytes

FWINFO_SIZE         = 0x20
FWINFO_SENTINEL = 0xDEADC0DE  # Sentinel value for firmware info
FWINFO_SENTINEL_OFFSET = 0x00  # Offset for firmware info sentinel
FWINFO_DEVICE_ID_OFFSET = 0x04  # Offset for device ID in the firmware info section
FWINFO_VERSION_OFFSET = 0x08  # Offset for firmware version in the firmware info section
FWINFO_LENGTH_OFFSET = 0x0C  # Offset for firmware length in the firmware info section
FW_SIGNATURE_OFFSET = VECTOR_TABLE_SIZE + FWINFO_SIZE

signing_key = 0x000102030405060708090A0B0C0D0E0F  # Example signing key, replace with actual key
signing_iv  = 0x00000000000000000000000000000000

signed_filename = "signed" + ".bin"  # Output filename for the signed firmware
signing_image_filename = "image-to-be-signed" + ".bin"
encrypted_image_filename = "encrypted-image" + ".enc"

# Create a temporary firmware image which removes the bootloader and zeros out
# the key slot.

if len(sys.argv) != 3:
    print("Usage: {} <firmware_image> <version no.HEX>".format(sys.argv[0]))
    sys.exit(1)

# Check if the input file exists, then open it and read its contents
if not os.path.isfile(sys.argv[1]):
    print("Error: Firmware image file '{}' does not exist.".format(sys.argv[1]))
    sys.exit(1)
with open(sys.argv[1], "rb") as f:
    f.seek(BOOTLOADER_SIZE) # Skip the bootloader section
    firmware_image = bytearray(f.read())
    f.close()

version_hex = int(sys.argv[2], 16)
struct.pack_into("<I", firmware_image, VECTOR_TABLE_SIZE + FWINFO_VERSION_OFFSET, version_hex)
struct.pack_into("<I", firmware_image, VECTOR_TABLE_SIZE + FWINFO_LENGTH_OFFSET, len(firmware_image))

# Apply AES encryption to everything
signing_image = firmware_image[VECTOR_TABLE_SIZE:VECTOR_TABLE_SIZE + AES_BLOCK_SIZE * 2]
signing_image += firmware_image[:VECTOR_TABLE_SIZE]
signing_image += firmware_image[VECTOR_TABLE_SIZE + AES_BLOCK_SIZE * 3:]

with open(signing_image_filename, "wb") as f:
    f.write(signing_image)
    f.close()

# openssl enc -aes-128-cbc -nosalt -K <key> -iv <IV> -in <input> -out <output
openssl_command = f"openssl enc -aes-128-cbc -nosalt -K {signing_key:032x} -iv {signing_iv:032x} -in {signing_image_filename} -out {encrypted_image_filename}"

subprocess.run(openssl_command, shell=True)

with open(encrypted_image_filename, "rb") as f:
    f.seek(-AES_BLOCK_SIZE, 2) # seek to the last AES block
    signature = f.read()
    f.close()

signature_text = "".join(f"{byte:02x}" for byte in signature)

# DEBUG: Print firmware image and info section 
print(f"Signed firmware version {sys.argv[2]}")
print(f"key        = {signing_key:032x}")
print(f"signature  = {signature_text}")
print(f"image size = {len(firmware_image):032x} = {len(firmware_image)} bytes")
# print("Firmware info section size: {} bytes".format(len(fw_info_section)))
# print("Firmware info: {}".format(fw_info_section))

os.remove(signing_image_filename)
# os.remove(encrypted_image_filename)

# write in signature
# TODO: Understand what the point of this is
firmware_image[FW_SIGNATURE_OFFSET:FW_SIGNATURE_OFFSET + AES_BLOCK_SIZE] = signature;

with open(signed_filename, "wb") as f:
    f.write(firmware_image)
    f.close()