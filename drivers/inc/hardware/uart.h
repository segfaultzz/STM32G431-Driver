#ifndef HARDWARE_UART_H
#define HARDWARE_UART_H

#include "hardware/err.h"
#include "stm32g4xx.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Polling UART driver for USART1, USART2, USART3, UART4 and LPUART1.
 * No interrupts or DMA. Timeouts are measured with the DWT cycle counter,
 * so they work before the FreeRTOS scheduler starts.
 *
 * uart_init() enables the peripheral clock but does NOT set up the pins:
 * configure TX/RX as alternate function with gpio_init() first.
 */

#define UART_WAIT_FOREVER UINT32_MAX

typedef enum {
  UART_PARITY_NONE,
  UART_PARITY_EVEN,
  UART_PARITY_ODD,
} uart_parity_t;

typedef enum {
  UART_STOP_1,
  UART_STOP_2,
} uart_stop_t;

/* Kernel clock that drives the baud generator. Values = RCC_CCIPR encoding */
typedef enum {
  UART_CLK_PCLK   = 0, /* PCLK2 for USART1, PCLK1 for the others (reset default) */
  UART_CLK_SYSCLK = 1,
  UART_CLK_HSI16  = 2, /* baud independent of the system clock setup */
  UART_CLK_LSE    = 3, /* 32.768 kHz, low-power LPUART up to 9600 baud */
} uart_clk_t;

typedef struct {
  uint32_t baud;
  uint8_t data_bits; /* 7 or 8 (parity, if any, is an extra bit on top) */
  uart_parity_t parity;
  uart_stop_t stop;
  uart_clk_t clock;
} uart_config_t;

/* 8 data bits, no parity, 1 stop bit, clocked from PCLK */
#define UART_CONFIG_8N1(b)                                                    \
  ((uart_config_t){.baud = (b), .data_bits = 8, .parity = UART_PARITY_NONE,  \
                   .stop = UART_STOP_1, .clock = UART_CLK_PCLK})

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enables, resets and configures a UART, then turns on TX and RX.
 *
 * The baud divider is computed from the kernel clock at the time of the
 * call: call it again after changing the clock tree with rcc_clock_config().
 *
 * @return ERR_OK, ERR_NULL_PTR, ERR_UART_BAD_INSTANCE, ERR_UART_BAD_FORMAT,
 *         ERR_UART_BAD_CLOCK (HSI16/LSE not running), ERR_UART_BAD_BAUD
 *         (divider out of range for this clock), ERR_TIMEOUT.
 */
err_t uart_init(USART_TypeDef *uart, const uart_config_t *config);

/** @brief Disables the UART, resets its registers and gates its clock. */
err_t uart_deinit(USART_TypeDef *uart);

/**
 * @brief Sends len bytes, blocking until each one is in the transmit register.
 * @param timeout_ms Limit for the whole transfer, or UART_WAIT_FOREVER.
 * @return ERR_OK, ERR_NULL_PTR, ERR_UART_BAD_INSTANCE, ERR_UART_NOT_ENABLED,
 *         ERR_UART_TX_TIMEOUT.
 */
err_t uart_write(USART_TypeDef *uart, const void *data, size_t len,
                 uint32_t timeout_ms);

/** @brief Waits until the last byte has fully left the TX pin. */
err_t uart_flush(USART_TypeDef *uart, uint32_t timeout_ms);

/**
 * @brief Reads one byte if available (never blocks).
 * @return ERR_OK, ERR_UART_RX_EMPTY if nothing arrived, or a line error:
 *         ERR_UART_PARITY / ERR_UART_FRAMING / ERR_UART_NOISE (the bad byte is
 *         discarded) or ERR_UART_OVERRUN (a byte was lost; the next read
 *         returns the byte still in the receiver). Error flags are cleared.
 */
err_t uart_getc(USART_TypeDef *uart, uint8_t *byte);

/**
 * @brief Reads exactly len bytes, blocking.
 * @param received Optional: number of bytes stored before returning.
 * @return ERR_OK, ERR_UART_RX_TIMEOUT, or the first line error (see uart_getc).
 */
err_t uart_read(USART_TypeDef *uart, void *buf, size_t len, uint32_t timeout_ms,
                size_t *received);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_UART_H */
