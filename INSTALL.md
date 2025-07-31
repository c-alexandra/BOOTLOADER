# Installation Guide

This guide provides detailed installation instructions for currently supported platforms. (This is a learning project, so I can only vouch for my own platform, MacOS)

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Platform Specific Requirements](#platform-specific-requirements)
3. [Hardware Setup](#hardware-setup)
4. [Development](#development)
5. [Troubleshooting](#troubleshooting)

## System Requirements

- **OS:** macOS (Sequioa 15.5+)
- **Node.js** (v23.11.0+)
- **ST-Link**

## Platform-Specific Requirements

### macOS

#### 1. Install prerequisites

Install **node.js** via homebrew  

```bash
# Install node js
brew install node

# Verify install
node --version
```

Install **st-link** via homebrew  

```bash
# Install st-link
brew install stlink

# Verify install
...
```

#### 2. Install project

```bash
# Clone repository
git clone https://github.com/c-alexandra/BOOTLOADER.git cd project-name

# Create virtual environment
...

# Add submodule to project, inside root project folder
git submodule add https://github.com/libopencm3/libopencm3.git

# Initialize submodule
git submodule update --init
```

## Hardware Setup

### 1. Identify your devices

```bash
# macOS
ls /dev/tty.usb* # Show connected serial devices
st-info --descr # Show connected ST device name
```

### 2. Test serial connection

```bash
# linux/macOS - Test with screen
screen /dev/tty.usbserial_etc 9600 # exit with 'ctrl+A', then 'K', then 'Y'
```

### 3. Hardware connections

Connect UART adapter RX to stm32 TX, and adapter TX to stm32 RX.  

Additional hardware connections are available in code documentation and currently change too rapidly for me to update at this time.

## Development

### ST-Utils interfacing

To begin an ST-Link debug session, start a GDB server from a terminal with  

```bash
st-util
```

Then, in another terminal  

```bash
arm-none-eab-gdb app/firmware.elf
target extended localhost:4242 # could instead 'tar ext :4242'
load
continue
```

### Building project

This project has been structured in such a way that the **Bootloader** and **Main Application** can be compiled separately.
This allows the firmware to change at will, without affecting the stable bootloader implementation.  

When main application **make** is ran, the bootloader and main application .bin files are packaged together. This resulting .bin may be loaded on to an STM32 device via main memory injection directly. That is,  
`st-flash write app/firmware.bin 0x08000000`  
will write the whole application implementation directly into device flash memory.  
**Note that `0x08000000` is correct for the STM32F446RE package specifically, but that memory blocks may vary by device.**

## Troubleshooting

### Intellisense not functioning in library headers

To configure intellisense for library headers, add hardware target to project configurations: that is, add to `.vscode/c_cpp_properties`, under `json defines`, the value of your given stm32 board. In my case, `STM32F`.

### ST-Link terminal commands

To restart/power cycle the chip,  
`st-flash reset`

To identify device,  
`st-info --descr`

To erase device flash memory,  
`st-flash erase`

To write binary file to flash memory on device,  

```bash
# st-flash write <binary location> <flash memory block start>
# eg...
st-flash write app/firmware.bin 0x08000000
```

### Permission issues with serial port

```bash
# serial port permission denied
sudo usermod -a -G dialout $USER # Linux
sudo dseditgroup -o edit -a $USER -t user wheel  # macOS (if needed)
```

### Communication timeouts or unexpected values in serial screen

1. Verify Baud rate settings
2. Check cable connections (particularly Tx->Rx, Rx->Tx)
3. Try hardware loopback (short adapter RX and TX pins)
