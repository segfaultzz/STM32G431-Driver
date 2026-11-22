#ifndef DEBUG_H
#define DEBUG_H

#include "hardware/err.h"
#include "hardware/uart.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Debug console on the NUCLEO-G431KB ST-LINK virtual COM port:
 * USART2, TX = PA2, RX = PA3, AF7. Open the board's /dev/ttyACM* at the
 * baud passed to debug_init() (8N1).
 */
#define DEBUG_UART      USART2
#define DEBUG_GPIO_PORT GPIOA
#define DEBUG_TX_PIN    GPIO_PIN_2
#define DEBUG_RX_PIN    GPIO_PIN_3
#define DEBUG_GPIO_AF   7U
#define DEBUG_BAUD      115200U

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sets up the console pins and UART. Call after rcc_clock_config()
 *        (the baud divider depends on the clock) and before printk().
 *        printk() output before this call is dropped.
 */
err_t debug_init(uint32_t baud);

/**
 * @brief printf-style output to the console. "\n" is sent as "\r\n".
 *        Messages longer than the internal buffer end with "...".
 *        Thread safe once the scheduler runs; don't call from ISRs.
 * @return Number of characters formatted (like printf), or -1 if the
 *         console isn't initialized.
 */
int printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/** @brief Raw output used by printk() and newlib's printf(). */
void debug_write(const char *buf, size_t len);

/** @brief Backend of pr_err(): prints location, message and the error's name/description. */
void printk_err(const char *file, int line, err_t err, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

#ifdef __cplusplus
}
#endif

/* Log helpers. Output: "[ERR] main.c:42 rcc_clock_config: ERR_RCC_HSE_TIMEOUT (0x0104) HSE ..." */
#define pr_info(fmt, ...) printk("[INF] " fmt "\n", ##__VA_ARGS__)
#define pr_warn(fmt, ...) printk("[WRN] " fmt "\n", ##__VA_ARGS__)
#define pr_err(err, ...)  printk_err(__FILE_NAME__, __LINE__, (err), __VA_ARGS__)

/*
 * Runs a call returning err_t, prints it with pr_err() if it failed, and
 * evaluates to the error so the caller can still react:
 *     if (ERR_CHECK(uart_init(USART1, &cfg)) != ERR_OK) { ... }
 */
#define ERR_CHECK(expr)                                                       \
  ({                                                                          \
    err_t err_check_ = (expr);                                                \
    if (err_check_ != ERR_OK) {                                               \
      pr_err(err_check_, "%s", #expr);                                        \
    }                                                                         \
    err_check_;                                                               \
  })

#endif /* DEBUG_H */
