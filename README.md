# BOOTLOADER

> **Status: Stable Relase v1.0.0**
>
> This project has reached its initial completion milestone. All core
> functionality is implemented, tested, and documented.

I used this project as a learning opportunity for implementing a rudimentary bootloader and packet communication protocol. This version of the project is inspired by [Low Byte Production's](https://www.youtube.com/@LowByteProductions) Bare Metal Programming series.  

In this project, I heavily utilize serial UART communication, various C data structures and algorithms, and JS scripts to implement an STM32 Nucleo bootloader.  
The bootloader launches before the main application, initiating an update sequence which searches for a sync packet through serial input. Upon receiving matched sync and ID packets, the bootloader wipes an area of flash memory, then writes a received firmware image into a designated memory block.  
After verifying firmware integrity, bootload sequence finishes and the updated main application is launched.

## Project Completion

I consider this project **feature-complete** for version 1.0. It succesfully:

- [x] Achieves all original design goals
- [x] Passes MVP testing
- [x] Includes functional documentation
- [x] Demonstrates stable hardware/software integration

## Future Development

See [CHANGELOG.md](CHANGELOG.md) for version history and planned release features.

Next Planned release: v1.1.0 with advanced firmware validation method.
