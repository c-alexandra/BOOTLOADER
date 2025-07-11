/*******************************************************************************
 * @file   shift-register.c
 * @author Camille Alexandra
 *
 * @brief  Shift register implementation for controlling debug LEDs
 *         using SPI1 peripheral & SN74HC595 shift register on STM32F446RE 
 ******************************************************************************/


#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "common.h"
#include "core/shift-register.h"


/**
 * @brief Initializes the SPI peripheral
 */
static void spi_setup(void) {
    rcc_periph_clock_enable(RCC_SPI1); // enable rcc for SPI1

    // configure GPIO for SS= NONE, SCK=PB3, MISO= NONE, MOSI=PB5
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO3 | GPIO5);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO3 | GPIO5); // set spi alternate functions

    // configure latch pin PB0 to SR RCLK
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0); 
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO0);

    gpio_clear(GPIOB, GPIO0); // set latch pin low

    rcc_periph_reset_pulse(RST_SPI1); // reset SPI1 peripheral

    /* set up SPI1 in Master Mode with:
     * clock baud rate: 1/8 of peripheral clock freq
     * clock polarity: idle high
     * clock phase: data sampled on second clock transition
     * data frame format: 8-bit
     * data transfer format: LSB first
     */
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32,
        SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, 
        SPI_CR1_DFF_8BIT, SPI_CR1_LSBFIRST); 

    /* Set NSS management to software, so we can control the SS pin manually
     * 
     * note: even if controlling GPIO ourselves through software, we still need
     * to set this bit to 1, otheriwse the SPI peripheral will not send data out
     */
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1); // set NSS pin high

    spi_enable(SPI1); // enable SPI1 peripheral
}

void shift_register_setup(void) {
    spi_setup(); // setup SPI peripheral for shift register communication
}

/**
 * @brief Shift out a byte of data to the debug LEDs using SPI
 * 
 * @param data The byte of data to send to the shift register
 */
void debug_led_shift_out_spi(uint8_t data) {
    gpio_clear(GPIOB, GPIO0); // set latch pin low to prepare for data transfer

    while (!(SPI_SR(SPI1) & SPI_SR_TXE)); // wait until transmit buffer is empty
    spi_send(SPI1, data); // send data byte over SPI
    while (!(SPI_SR(SPI1) & SPI_SR_TXE)); // wait until transmit buffer is empty
    while (SPI_SR(SPI1) & SPI_SR_BSY); // wait until SPI is not busy

    gpio_set(GPIOB, GPIO0); // set latch pin high to latch data into shift register
    system_delay(1); // small delay to ensure data is latched
    gpio_clear(GPIOB, GPIO0); // set latch pin low again to prepare for next data transfer
}