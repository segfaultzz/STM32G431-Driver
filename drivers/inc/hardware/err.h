#ifndef ERR_H
#define ERR_H

#include <stdint.h>

/*
 * Error codes: 0xMMCC
 *   MM = module (00 common, 01 RCC, 02 GPIO, 03 UART)
 *   CC = error within that module
 *
 * Every function returns ERR_OK (0) on success, so `if (err != ERR_OK)`
 * works for all of them. err_name() / err_desc() turn a code into text.
 * When adding a code, also add it to the switch in err.c (the compiler
 * warns if one is missing).
 */
typedef enum {
  /* ---- Common 0x00xx ---- */
  ERR_OK                   = 0x0000,
  ERR_INVALID_PARAM        = 0x0001,
  ERR_TIMEOUT              = 0x0002,
  ERR_NULL_PTR             = 0x0003,
  ERR_BUSY                 = 0x0004,
  ERR_NOT_INITIALIZED      = 0x0005,
  ERR_UNSUPPORTED          = 0x0006,

  /* ---- RCC 0x01xx ---- */
  ERR_RCC_UNKNOWN_PERIPH   = 0x0101,
  ERR_RCC_NO_RESET_BIT     = 0x0102,
  ERR_RCC_HSI16_TIMEOUT    = 0x0103,
  ERR_RCC_HSE_TIMEOUT      = 0x0104,
  ERR_RCC_HSI48_TIMEOUT    = 0x0105,
  ERR_RCC_LSI_TIMEOUT      = 0x0106,
  ERR_RCC_LSE_TIMEOUT      = 0x0107,
  ERR_RCC_PLL_TIMEOUT      = 0x0108,
  ERR_RCC_SWITCH_TIMEOUT   = 0x0109,
  ERR_RCC_FLASH_LATENCY    = 0x010A,
  ERR_RCC_VOS_TIMEOUT      = 0x010B,
  ERR_RCC_OSC_IN_USE       = 0x010C,
  ERR_RCC_HSE_FREQ         = 0x010D,
  ERR_RCC_BAD_SOURCE       = 0x010E,
  ERR_RCC_BAD_PRESCALER    = 0x010F,
  ERR_RCC_PLL_PARAM        = 0x0110,
  ERR_RCC_PLL_INPUT_RANGE  = 0x0111,
  ERR_RCC_PLL_VCO_RANGE    = 0x0112,
  ERR_RCC_PLL_OUTPUT_RANGE = 0x0113,
  ERR_RCC_HCLK_TOO_HIGH    = 0x0114,

  /* ---- GPIO 0x02xx ---- */
  ERR_GPIO_NO_PINS         = 0x0201,
  ERR_GPIO_BAD_MODE        = 0x0202,
  ERR_GPIO_BAD_PULL        = 0x0203,
  ERR_GPIO_BAD_OTYPE       = 0x0204,
  ERR_GPIO_BAD_SPEED       = 0x0205,
  ERR_GPIO_BAD_AF          = 0x0206,
  ERR_GPIO_LOCK_FAILED     = 0x0207,

  /* ---- UART 0x03xx ---- */
  ERR_UART_BAD_INSTANCE    = 0x0301,
  ERR_UART_BAD_BAUD        = 0x0302,
  ERR_UART_BAD_FORMAT      = 0x0303,
  ERR_UART_BAD_CLOCK       = 0x0304,
  ERR_UART_NOT_ENABLED     = 0x0305,
  ERR_UART_TX_TIMEOUT      = 0x0306,
  ERR_UART_RX_TIMEOUT      = 0x0307,
  ERR_UART_RX_EMPTY        = 0x0308,
  ERR_UART_OVERRUN         = 0x0309,
  ERR_UART_FRAMING         = 0x030A,
  ERR_UART_PARITY          = 0x030B,
  ERR_UART_NOISE           = 0x030C,
} err_t;

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Name of the code, e.g. "ERR_RCC_HSE_TIMEOUT". Never NULL. */
const char *err_name(err_t err);

/** @brief One-line human description, e.g. "HSE oscillator did not start". Never NULL. */
const char *err_desc(err_t err);

/** @brief Module part of the code: "COMMON", "RCC", "GPIO", "UART". Never NULL. */
const char *err_module(err_t err);

#ifdef __cplusplus
}
#endif

#endif /* ERR_H */
