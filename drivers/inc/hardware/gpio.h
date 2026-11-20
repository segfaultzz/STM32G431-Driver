#ifndef HARDWARE_GPIO_H
#define HARDWARE_GPIO_H

#include "hardware/err.h"
#include "stm32g4xx.h"
#include <stdint.h>

/* Pin masks: OR them together, pass as uint16_t */
#define GPIO_PIN_0 ((uint16_t)0x0001)
#define GPIO_PIN_1 ((uint16_t)0x0002)
#define GPIO_PIN_2 ((uint16_t)0x0004)
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)
#define GPIO_PIN_7 ((uint16_t)0x0080)
#define GPIO_PIN_8 ((uint16_t)0x0100)
#define GPIO_PIN_9 ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define GPIO_PIN_14 ((uint16_t)0x4000)
#define GPIO_PIN_15 ((uint16_t)0x8000)

typedef enum {
  GPIO_MODE_INPUT,
  GPIO_MODE_OUTPUT,
  GPIO_MODE_AF,
  GPIO_MODE_ANALOG
} gpio_mode_t;

typedef enum { GPIO_PULL_NONE, GPIO_PULL_UP, GPIO_PULL_DOWN } gpio_pull_t;
typedef enum { GPIO_OTYPE_PUSH_PULL, GPIO_OTYPE_OPEN_DRAIN } gpio_otype_t;

typedef enum {
  GPIO_SPEED_LOW,
  GPIO_SPEED_MEDIUM,
  GPIO_SPEED_HIGH,
  GPIO_SPEED_VERY_HIGH
} gpio_speed_t;

typedef struct {
  uint16_t pins;  // Bitmask of pins to configure (e.g., GPIO_PIN_0 | GPIO_PIN_1)
  gpio_mode_t mode;
  gpio_pull_t pull;
  gpio_otype_t otype;
  gpio_speed_t speed;
  uint8_t af;
} gpio_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the specified GPIO pins with the given configuration.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param config: Pointer to the configuration structure.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_init(GPIO_TypeDef *port, const gpio_config_t *config);

/**
 * @brief Returns the specified GPIO pins to their reset state (undoes gpio_init).
 *
 * Mode analog, no pull, push-pull, low speed, AF0, output level low.
 * Analog is the lowest-power state for an unused pin.
 *
 * @param port Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pins Bitmask of the pins to reset (e.g., GPIO_PIN_0 | GPIO_PIN_1).
 * @return ERR_OK if successful, ERR_INVALID_PARAM if port is NULL or pins is 0.
 *
 * @warning Don't reset PA13/PA14 (SWDIO/SWCLK): the debugger loses the chip
 *          until the next connect-under-reset.
 * @note Pins locked with gpio_lock_pin() keep their configuration until reset.
 */
err_t gpio_reset_pin(GPIO_TypeDef *port, uint16_t pins);

/**
 * @brief Sets the mode of the specified GPIO pins.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pins: Bitmask of the pins to configure (e.g., GPIO_PIN_0 |
 * GPIO_PIN_1).
 * @param mode: The mode to set.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_set_mode(GPIO_TypeDef *port, uint16_t pins, gpio_mode_t mode);

/**
 * @brief Sets the pull configuration of the specified GPIO pins.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pins: Bitmask of the pins to configure (e.g., GPIO_PIN_0 |
 * GPIO_PIN_1).
 * @param pull: The pull configuration to set.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_set_pull(GPIO_TypeDef *port, uint16_t pins, gpio_pull_t pull);
/**
 * @brief Sets the output type of the specified GPIO pins.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pins: Bitmask of the pins to configure (e.g., GPIO_PIN_0 |
 * GPIO_PIN_1).
 * @param otype: The output type to set (GPIO_OTYPE_PUSH_PULL or
 * GPIO_OTYPE_OPEN_DRAIN).
 * @return ERR_OK if successful, or an error code otherwise.
 */

err_t gpio_set_output_type(GPIO_TypeDef *port, uint16_t pins,
                           gpio_otype_t otype);

/**
 * @brief Sets the output speed of the specified GPIO pins.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pins: Bitmask of the pins to configure (e.g., GPIO_PIN_0 |
 * GPIO_PIN_1).
 * @param speed: The output speed to set.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_set_output_speed(GPIO_TypeDef *port, uint16_t pins,
                            gpio_speed_t speed);
/**
 * @brief Sets the alternate function of the specified GPIO pins.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pins: Bitmask of the pins to configure (e.g., GPIO_PIN_0 |
 * GPIO_PIN_1).
 * @param af: The alternate function to set.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_set_af(GPIO_TypeDef *port, uint16_t pins, uint8_t af);
/**
 * @brief Writes a value to the specified GPIO pin.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pins: Bitmask of the pins to write (e.g., GPIO_PIN_0 |
 * GPIO_PIN_1).
 * @param value: The value to write.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_write_pin(GPIO_TypeDef *port, uint16_t pins, uint16_t value);
/**
 * @brief Reads the value of the specified GPIO pin.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pin: The pin to read.
 * @return The value of the pin.
 */
uint32_t gpio_read_pin(GPIO_TypeDef *port, uint16_t pin);
/**
 * @brief Toggles the value of the specified GPIO pin.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pin: The pin to toggle.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_toggle_pin(GPIO_TypeDef *port, uint16_t pin);
/**
 * @brief Locks the specified GPIO pin.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param pin: The pin to lock.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_lock_pin(GPIO_TypeDef *port, uint16_t pin);
/**
 * @brief Writes a value to the specified GPIO port.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param value: The value to write.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_write_port(GPIO_TypeDef *port, uint16_t value);
/**
 * @brief Reads the value of the specified GPIO port.
 * @param port: Pointer to the GPIO port (e.g., GPIOA, GPIOB, etc.).
 * @param value: Pointer to store the read value.
 * @return ERR_OK if successful, or an error code otherwise.
 */
err_t gpio_read_port(GPIO_TypeDef *port, uint16_t *value);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_GPIO_H */
