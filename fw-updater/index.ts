import {SerialPort} from 'serialport';
import * as path from 'path'; // importing path module for file paths
import * as fs from 'fs/promises'; // importing async file system module for reading files
import { time } from 'console';
import { write } from 'fs';

// Constants for the packet protocol
const PACKET_LENGTH_BYTES   = 1;
const PACKET_DATA_BYTES     = 16;
const PACKET_CRC_BYTES      = 1;
const PACKET_CRC_INDEX      = PACKET_LENGTH_BYTES + PACKET_DATA_BYTES;
const PACKET_LENGTH         = PACKET_LENGTH_BYTES + PACKET_DATA_BYTES + PACKET_CRC_BYTES;

const PACKET_ACK_DATA0      = 0x15;
const PACKET_RETX_DATA0     = 0x19;

const FLASH_BASE             = (0x08000000); // Flash memory base address
const BOOTLOADER_SIZE        = (0x00008000); // 32kB bootloader size
const MAIN_APP_START_ADDRESS = (0x08008000); // 32kB bootloader, so main app starts at 0x08008000
const VECTOR_TABLE_SIZE      = (0x000001B0); // 2080 bytes, which is the size of the vector table
const FIRMWARE_INFO_SIZE     = (8 * 4);

const BL_PACKET_SYNC_OBSERVED_DATA0      = (0x20);
const BL_PACKET_FW_UPDATE_REQUEST_DATA0  = (0x31);
const BL_PACKET_FW_UPDATE_RESPONSE_DATA0 = (0x37);
const BL_PACKET_DEVICE_ID_REQUEST_DATA0  = (0x3C);
const BL_PACKET_DEVICE_ID_RESPONSE_DATA0 = (0x3F);
const BL_PACKET_FW_LENGTH_REQUEST_DATA0  = (0x42);
const BL_PACKET_FW_LENGTH_RESPONSE_DATA0 = (0x45);
const BL_PACKET_READY_FOR_DATA_DATA0     = (0x48);
const BL_PACKET_UPDATE_SUCCESS_DATA0     = (0x54);
const BL_PACKET_NACK_DATA0               = (0x99);

const FWINFO_ADDRESS        = (FLASH_BASE + BOOTLOADER_SIZE + VECTOR_TABLE_SIZE)
const FWINFO_VALIDATE_FROM  = (VECTOR_TABLE_SIZE + FIRMWARE_INFO_SIZE)

const FWINFO_SENTINEL_OFFSET  = (VECTOR_TABLE_SIZE + (0 * 4));
const FWINFO_DEVICE_ID_OFFSET = (VECTOR_TABLE_SIZE + (1 * 4));
const FWINFO_VERSION_OFFSET   = (VECTOR_TABLE_SIZE + (2 * 4));
const FWINFO_LENGTH_OFFSET    = (VECTOR_TABLE_SIZE + (3 * 4));
const FWINFO_RESERVED0_OFFSET = (VECTOR_TABLE_SIZE + (4 * 4));
const FWINFO_RESERVED1_OFFSET = (VECTOR_TABLE_SIZE + (5 * 4));
const FWINFO_RESERVED2_OFFSET = (VECTOR_TABLE_SIZE + (6 * 4));
const FWINFO_RESERVED3_OFFSET = (VECTOR_TABLE_SIZE + (7 * 4));
const FWINFO_CRC32_OFFSET     = (VECTOR_TABLE_SIZE + (9 * 4));
const FWINFO_SENTINEL         = (0xDEADC0DE) // Example sentinel value to identify firmware info structure


// Validation constants
const DEVICE_ID = (0xA3); // arbitrary device id used to identify for fw uconst

const SYNC_SEQ = Buffer.from([0xC4, 0x55, 0x7E, 0x10]);

const DEFAULT_TIMEOUT = (5000);  // default timeout at 5s
const SHORT_TIMEOUT   = (1000);  // short timeout at 1s
const LONG_TIMEOUT    = (15000); // long timeout at 15s

// Bootloader constants
const BL_SIZE = 0x8000; // 32kB bootloader size

// Details about the serial port connection
// const serialPath1           = "/dev/tty.usbmodem21401";
const serialPath2           = "/dev/tty.usbserial-B00001TO";
const baudRate              = 115200;

// CRC8 implementation
const crc8 = (data: Buffer | Array<number>) => {
  let crc = 0;

  for (const byte of data) {
    crc = (crc ^ byte) & 0xff;
    for (let i = 0; i < 8; i++) {
      if (crc & 0x80) {
        crc = ((crc << 1) ^ 0x07) & 0xff;
      } else {
        crc = (crc << 1) & 0xff;
      }
    }
  }

  return crc;
};

// CRC32 implementation
const crc32 = (data: Buffer, length: number) => {
  let byte;
  let crc = 0xffffffff;
  let mask;

  for (let i = 0; i < length; i++) {
    byte = data[i];
    crc = (crc ^ byte) >>> 0;

    for (let j = 0; j < 8; j++) {
      mask = (-(crc & 1)) >>> 0;
      crc = ((crc >>> 1) ^ (0xedb88320 & mask)) >>> 0;
    }
  }

  return (~crc) >>> 0;
}

// Async delay function, which gives the event loop time to process outside input
const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

class Logger {
  static info(message: string) {console.log(`[.] ${message}`); }
  static success(message: string) {console.log(`[$] ${message}`); }
  static error(message: string) {console.error(`[!] ${message}`); }
}

// Class for serialising and deserialising packets
class Packet {
  length: number;
  data: Buffer;
  crc: number;

  static retx = new Packet(1, Buffer.from([PACKET_RETX_DATA0])).toBuffer();
  static ack = new Packet(1, Buffer.from([PACKET_ACK_DATA0])).toBuffer();

  constructor(length: number, data: Buffer, crc?: number) {
    this.length = length;
    this.data = data;

    const bytesToPad = PACKET_DATA_BYTES - this.data.length;
    const padding = Buffer.alloc(bytesToPad).fill(0xff);
    this.data = Buffer.concat([this.data, padding]);

    if (typeof crc === 'undefined') {
      this.crc = this.computeCrc();
    } else {
      this.crc = crc;
    }
  }

  computeCrc() {
    const allData = [this.length, ...this.data];
    return crc8(allData);
  }

  toBuffer() {
    return Buffer.concat([ Buffer.from([this.length]), this.data, Buffer.from([this.crc]) ]);
  }

  isSingleBytePacket(byte: number) {
    if (this.length !== 1) return false;
    if (this.data[0] !== byte) return false;
    for (let i = 1; i < PACKET_DATA_BYTES; i++) {
      if (this.data[i] !== 0xff) return false;
    }
    return true;
  }

  isAck() {
    return this.isSingleBytePacket(PACKET_ACK_DATA0);
  }

  isRetx() {
    return this.isSingleBytePacket(PACKET_RETX_DATA0);
  }

  static createSingleBytePacket(byte: number) {
    return new Packet(1, Buffer.from([byte]));
  }
}

// Serial port instance
const uart = new SerialPort({ path: serialPath2, baudRate });

// Packet buffer
let packets: Packet[] = [];

let lastPacket: Buffer = Packet.ack;
const writePacket = (packet: Buffer) => {
  uart.write(packet);
  lastPacket = packet;
};

// Serial data buffer, with a splice-like function for consuming data
let rxBuffer = Buffer.from([]);
const consumeFromBuffer = (n: number) => {
  const consumed = rxBuffer.slice(0, n);
  rxBuffer = rxBuffer.slice(n);
  return consumed;
}

// This function fires whenever data is received over the serial port. The whole
// packet state machine runs here.
uart.on('data', data => {
  // Logger.info(`Received ${data.length} bytes through uart`);
  // Add the data to the packet
  rxBuffer = Buffer.concat([rxBuffer, data]);

  // Can we build a packet?
  while (rxBuffer.length >= PACKET_LENGTH) {
    // Logger.info(`Building a packet`);
    const raw = consumeFromBuffer(PACKET_LENGTH);
    const packet = new Packet(raw[0], raw.slice(1, 1+PACKET_DATA_BYTES), raw[PACKET_CRC_INDEX]);
    const computedCrc = packet.computeCrc();

    // Need retransmission?
    if (packet.crc !== computedCrc) {
      // Logger.error(`CRC failed, computed 0x${computedCrc.toString(16)}, got 0x${packet.crc.toString(16)}`);
      writePacket(Packet.retx);
      continue;
    }

    // Are we being asked to retransmit?
    if (packet.isRetx()) {
      // Logger.info(`Retransmitting last packet`);
      writePacket(lastPacket);
      continue;
    }

    // If this is an ack, move on
    if (packet.isAck()) {
      // Logger.info(`It was an ack, nothing to do`);
      continue;
    }

    // If this is a nack, exit program
    if (packet.isSingleBytePacket(BL_PACKET_NACK_DATA0)) {
      Logger.error(`Received NACK packet, exiting...`)
      process.exit(1);
    }

    // Otherwise write the packet in to the buffer, and send an ack
    // Logger.info(`Storing packet and ack'ing`);
    packets.push(packet);
    writePacket(Packet.ack);
  }
});

// Function to allow us to await a packet
const waitForPacket = async (timeout = DEFAULT_TIMEOUT) => {
  let timeWaited = 0;

  while (packets.length < 1) {
    await delay(1);
    timeWaited += 1;

    if (timeWaited >= timeout) {
      // Logger.error(`Timeout waiting for packet after ${timeout}ms`);
      throw Error('Timed out waiting for packet');  
      // process.exit(1);
    }
  }

  return packets.splice(0, 1)[0]; // packets[0]
}

const waitForSingleBytePacket = (byte: number, timeout = DEFAULT_TIMEOUT) => (
  waitForPacket(timeout)
    .then(packet => {
      if (packet.length != 1 || packet.data[0] != byte) {
        throw new Error(`Expected single byte packet with data 0x${byte.toString(16)}, got packet: ${packet}`);
      }
    })
    .catch(err => {
      Logger.error(`Error waiting for single byte packet: ${err.message}`);
      console.log(rxBuffer);
      console.log(packets);
      process.exit(1);
    })
);

// console.log(Packet.ack) // DEBUG

const syncWithBootloader = async (timeout = DEFAULT_TIMEOUT) => {
  let timeWaited = 0;

  while (true) {
    uart.write(SYNC_SEQ);
    await delay(SHORT_TIMEOUT); // device should respond within < 1s
    timeWaited += SHORT_TIMEOUT;

    if (packets.length > 0) { // if we have a packet, then we can assume we are in sync}
      const packet = packets.splice(0, 1)[0];

      if (packet.isSingleBytePacket(BL_PACKET_SYNC_OBSERVED_DATA0)) {
        Logger.info("Bootloader sync observed");
        return;
      } else {
        Logger.error(`Bootloader sync failed, got packet: ${packet}`);
        process.exit(1);
      }

    }

    // if we haven't received packet after given timeout, then exit
    if (timeWaited >= timeout) {
      Logger.error(`Bootloader sync timed out after ${timeout}ms`);
      process.exit(1);
    }
  }
}

const waitForFlashErase = (timeout = LONG_TIMEOUT) => {
  let timeWaited = 0;
  let totalTimeWaited = 0;

  while (packets.length < 0) {
    if (timeWaited >= 1000) {
      Logger.info(`Waiting for flash erase to complete...`)
      timeWaited = 0;
    }
    timeWaited += 1;
    totalTimeWaited += 1;
  }
  
  // const packet = packets.splice(0, 1)[0];
  // if (packet.isSingleBytePacket(BL_PACKET_READY_FOR_DATA_DATA0)) {
  //   Logger.success("Flash erase complete, ready for data");
  //   return;
  // } else {
  //   Logger.error(`Flash erase failed, got packet: ${packet}`);
  //   process.exit(1);
  // }
  }

// Do everything in an async function so we can have loops, awaits etc
const main = async () => {
  if (process.argv.length < 3) {
    console.log(`usage: ${process.argv[0]} <signed firmware>`);
    process.exit(1);
  }
  const firmwareFilename = process.argv[2];

  // calculate the firmware length
  Logger.info('Reading firmware image, calculating firmware length...');
  // throw away the bootloader part of the firmware image
  const fwImage = await fs.readFile(path.join(process.cwd(), firmwareFilename));
  const fwLength = fwImage.length;
  Logger.success(`Firmware length is ${fwLength} bytes`);

  // Start the bootloader update process

  // Begin by attempting serial sync with bootloader
  Logger.info('Attempting to sync with bootloader...');
  await syncWithBootloader();
  Logger.success('Bootloader sync successful!');

  // Sync successful, now request for firmware update
  Logger.info('Requesting firmware update...');
  const fwUpdatePacket = Packet.createSingleBytePacket(BL_PACKET_FW_UPDATE_REQUEST_DATA0);
  writePacket(fwUpdatePacket.toBuffer());
  await waitForSingleBytePacket(BL_PACKET_FW_UPDATE_RESPONSE_DATA0);
  Logger.success('Firmware update request successful...');

  // If request found, validate firmware device ID
  Logger.info('Awaiting device ID request...');
  await waitForSingleBytePacket(BL_PACKET_DEVICE_ID_REQUEST_DATA0);
  Logger.success('Device ID request received, sending device ID...');
  const deviceID = fwImage[FWINFO_DEVICE_ID_OFFSET];
  const deviceIdPacket = new Packet(2, Buffer.from([BL_PACKET_DEVICE_ID_RESPONSE_DATA0, deviceID]));
  writePacket(deviceIdPacket.toBuffer());
  // Logger.success('Device ID requested and validated...');
  Logger.info(`Device ID ${deviceID.toString(16)} sent...`); // formats in hex

  // Receive firmware length request, then send calculated firmware length
  Logger.info('Awaiting firmware length request...');
  await waitForSingleBytePacket(BL_PACKET_FW_LENGTH_REQUEST_DATA0);
  Logger.success('Firmware length request received...');
  // 1 byte for packet tag, 4 bytes for little-endian uint32 firmware length
  const fwLengthPacketBuffer = Buffer.alloc(5);
  fwLengthPacketBuffer[0] = BL_PACKET_FW_LENGTH_RESPONSE_DATA0; // packet tag
  fwLengthPacketBuffer.writeUInt32LE(fwLength, 1); // firmware length
  const fwLengthPacket = new Packet(5, fwLengthPacketBuffer);
  writePacket(fwLengthPacket.toBuffer());
  Logger.info('Sending firmware length...');

  // at this point, bootloader should be erasing main application flash
  Logger.info('Main application erasing...');
  // TODO: Replace with loop
  // await waitForFlashErase();
  // Logger.success('Main application flash erased, ready for data...');

  // Now we can start sending the firmware data
  let bytesWritten = 0;
  while (bytesWritten < fwLength) {
    await waitForSingleBytePacket(BL_PACKET_READY_FOR_DATA_DATA0);

    const dataBytes = fwImage.slice(bytesWritten, bytesWritten + PACKET_DATA_BYTES);
    const dataLength = dataBytes.length;
    const packet = new Packet(dataLength, dataBytes);

    writePacket(packet.toBuffer());
    bytesWritten += dataLength;

    Logger.info(`Writing ${dataLength} bytes (${bytesWritten}/${fwLength})...`);
  }

  await waitForSingleBytePacket(BL_PACKET_UPDATE_SUCCESS_DATA0);
  Logger.success('Firmware update successful!');
}

main()
  // .catch(e => {throw e})
  .finally(() => uart.close());