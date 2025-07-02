import {SerialPort} from 'serialport';

// Constants for the packet protocol
const PACKET_LENGTH_BYTES   = 1;
const PACKET_DATA_BYTES     = 16;
const PACKET_CRC_BYTES      = 1;
const PACKET_CRC_INDEX      = PACKET_LENGTH_BYTES + PACKET_DATA_BYTES;
const PACKET_LENGTH         = PACKET_LENGTH_BYTES + PACKET_DATA_BYTES + PACKET_CRC_BYTES;

const PACKET_ACK_DATA0      = 0x15;
const PACKET_RETX_DATA0     = 0x19;

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

const DEVICE_ID = (0xA3); // arbitrary device id used to identify for fw uconst

const SYNC_SEQ = Buffer.from([0xC4, 0x55, 0x7E, 0x10]);

const DEFAULT_TIMEOUT = (5000);  // default timeout at 5s
const SHORT_TIMEOUT   = (1000);  // short timeout at 1s
const LONG_TIMEOUT    = (15000); // long timeout at 15s

// Details about the serial port connection
// const serialPath1            = "/dev/tty.usbmodem21401";
const serialPath2            = "/dev/tty.usbserial-B00001TO";
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
  if (rxBuffer.length >= PACKET_LENGTH) {
    // Logger.info(`Building a packet`);
    const raw = consumeFromBuffer(PACKET_LENGTH);
    const packet = new Packet(raw[0], raw.slice(1, 1+PACKET_DATA_BYTES), raw[PACKET_CRC_INDEX]);
    const computedCrc = packet.computeCrc();

    // Need retransmission?
    if (packet.crc !== computedCrc) {
      // Logger.error(`CRC failed, computed 0x${computedCrc.toString(16)}, got 0x${packet.crc.toString(16)}`);
      writePacket(Packet.retx);
      return;
    }

    // Are we being asked to retransmit?
    if (packet.isRetx()) {
    // Logger.info(`Retransmitting last packet`);
    writePacket(lastPacket);
      return;
    }

    // If this is an ack, move on
    if (packet.isAck()) {
      // Logger.info(`It was an ack, nothing to do`);
      return;
    }

    // Otherwise write the packet in to the buffer, and send an ack
    // Logger.info(`Storing packet and ack'ing`);
    packets.push(packet);
    writePacket(Packet.ack);
  }
});

// Function to allow us to await a packet
const waitForPacket = async () => {
  while (packets.length < 1) {
    await delay(1);
  }

  // const packet = packets[0];
  // packets = packets.slice(1);
  // return packet;

  // splice is a bit more efficient than slice
  // it removes the elements from the array and returns it
  // can replace all the previous with this:
  return packets.splice(0, 1)[0]; // packets[0]
}

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

// Do everything in an async function so we can have loops, awaits etc
const main = async () => {
  Logger.info('Attempting to sync with bootloader...');
  await syncWithBootloader(DEFAULT_TIMEOUT);
  Logger.success('Bootloader sync successful!');
}

main();