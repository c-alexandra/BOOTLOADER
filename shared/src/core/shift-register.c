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
#include "core/system.h"

/*******************************************************************************
 * @brief Initializes the SPI peripheral
 * 
 * @param sr Pointer to the structure containing shift register configuration
 ******************************************************************************/
static void spi1_setup(const ShiftRegister8_t *sr) {
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_SPI1); // enable rcc for SPI1

    // configure GPIO for SS= NONE, SCK=PB3, MISO= NONE, MOSI=PB5
    gpio_mode_setup(sr->gpio_port, GPIO_MODE_AF, 
        GPIO_PUPD_PULLDOWN, sr->srclk_pin | sr->ser_pin);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO3 | GPIO5); // set spi alternate functions

    // configure latch pin PB0 to SR RCLK
    gpio_mode_setup(sr->gpio_port, 
        GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, sr->rclk_pin); 
    gpio_set_output_options(sr->gpio_port, 
        GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, sr->rclk_pin);

    gpio_clear(sr->gpio_port, sr->rclk_pin); // set latch pin low

    rcc_periph_reset_pulse(RST_SPI1); // reset SPI1 peripheral

    /* set up SPI1 in Master Mode with:
     * clock baud rate: 1/32 of peripheral clock freq
     * clock polarity: idle high
     * clock phase: data sampled on second clock transition
     * data frame format: 8-bit
     * data transfer format: MSB first
     */
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32,
        SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, 
        SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST); 

    /* Set NSS management to software, so we can control the SS pin manually
     * 
     * note: even if controlling GPIO ourselves through software, we still need
     * to set this bit to 1, otheriwse the SPI peripheral will not send data out
     */
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1); // set NSS pin high

    spi_enable(SPI1); // enable SPI1 peripheral
}

/*******************************************************************************
 * @brief public interface for SP1 setup
 * 
 * @param sr Pointer to the structure containing shift register configuration
 *****************************************************************************/
void shift_register_setup(const ShiftRegister8_t *sr) {
    spi1_setup(sr); // setup SPI peripheral for shift register communication
}

/*******************************************************************************
 * @brief reset SPI peripheral and relevant GPIO to default known state
 ******************************************************************************/
void shift_register_teardown(void) {
    spi_disable(SPI1); // disable SPI1 peripheral
    rcc_periph_reset_pulse(RST_SPI1); // reset SPI1 peripheral
    rcc_periph_clock_disable(RCC_SPI1); // disable rcc for SPI1
    gpio_mode_setup(SR1_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, 
        SR1_DATA_PIN | SR1_CLOCK_PIN | SR1_LATCH_PIN); // set pins to input mode

    // rcc_periph_clock_disable(RCC_GPIOB);
}

/*******************************************************************************
 * @brief Shift out a byte of data to the debug LEDs using SPI
 * 
 * @param sr Pointer to the structure containing shift register configuration
 * @param data The byte of data to send to the shift register
 ******************************************************************************/
static void debug_led_shift_out_spi(ShiftRegister8_t *sr, uint8_t data) {
    // set latch pin low to prepare for data transfer
    gpio_clear(sr->gpio_port, sr->rclk_pin);

    while (!(SPI_SR(SPI1) & SPI_SR_TXE)); // wait until transmit buffer is empty
    spi_send(SPI1, data); // send data byte over SPI
    while (!(SPI_SR(SPI1) & SPI_SR_TXE)); // wait until transmit buffer is empty
    while (SPI_SR(SPI1) & SPI_SR_BSY); // wait until SPI is not busy

    // set latch pin high to latch data into shift register
    gpio_set(sr->gpio_port, sr->rclk_pin); 

    system_delay(1); // small delay to ensure data is latched

    // set latch pin low again to prepare for next data transfer
    gpio_clear(sr->gpio_port, sr->rclk_pin); 

    // update the led_state in the ShiftRegister8_t structure
    sr->led_state = data; 
}

/*******************************************************************************
 * @brief Set the pattern of LEDs in the shift register directly 
 * 
 * @param sr Pointer to the structure containing shift register configuration
 * @param pattern The byte pattern to set in the shift register 
 *        (e.g., 0xFF for all LEDs on)
 ******************************************************************************/
void shift_register_set_pattern(ShiftRegister8_t *sr, uint8_t pattern) {

    if (sr->led_state == pattern) {
        return; // no change in pattern, do nothing
    }
    // Shift out the pattern to the shift register
    debug_led_shift_out_spi(sr, pattern);
    
    // Update the led_state in the SR8_t structure
    // sr->led_state = pattern;
}

/*******************************************************************************
 * @brief Set the state of a specific LED in the shift register
 * 
 * @param sr Pointer to the structure containing SR configuration
 * @param led The index of the LED to set (0-7)
 * @param state The state to set the LED to (true for on, false for off)
 ******************************************************************************/
void shift_register_set_led(ShiftRegister8_t *sr, uint8_t led, bool state) {
    uint8_t next_state = sr->led_state;

    // Set the specified LED in the shift register state
    if (led > sr->num_outputs) {
        return;
    }

    if (state) {
        next_state = sr->led_state | (1 << led); // set bit to 1
    } else {
        next_state = sr->led_state & ~(1 << led); // set bit to 0
    }
    shift_register_set_pattern(sr, next_state); // update shift register  
}

/*******************************************************************************
 * @brief Advance the shift register state by shifting left and wrapping around
 * 
 * @param sr Pointer to the structure containing SR configuration
 * @note This function shifts the current LED state left by one position,
 *       wrapping around to the first LED if reaching the last.
 ******************************************************************************/
void shift_register_advance(ShiftRegister8_t *sr) {
    uint8_t end = (1 << sr->num_outputs) - 1; // last LED index
    uint8_t next_pattern = (sr->led_state << 1) & end; // shift left and wrap

    if (next_pattern == 0) {
        next_pattern = 1; // reset to first LED if all LEDs are off
    }
    shift_register_set_pattern(sr, next_pattern); // update shift register
}
