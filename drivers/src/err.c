#include "hardware/err.h"

typedef struct {
    const char *name;
    const char *desc;
} err_info_t;

/*
 * One switch for every code. There is deliberately no `default:` label, so
 * -Wall (-Wswitch) warns when a code in err.h is missing here.
 */
#define E(code, text) case code: return (err_info_t){ #code, text }

static err_info_t lookup(err_t err){
    switch(err){
    /* Common */
    E(ERR_OK,                   "success");
    E(ERR_INVALID_PARAM,        "invalid parameter");
    E(ERR_TIMEOUT,              "operation timed out");
    E(ERR_NULL_PTR,             "NULL pointer argument");
    E(ERR_BUSY,                 "resource busy");
    E(ERR_NOT_INITIALIZED,      "not initialized");
    E(ERR_UNSUPPORTED,          "not supported");

    /* RCC */
    E(ERR_RCC_UNKNOWN_PERIPH,   "unknown peripheral ID");
    E(ERR_RCC_NO_RESET_BIT,     "peripheral has no reset bit (RTCAPB, WWDG)");
    E(ERR_RCC_HSI16_TIMEOUT,    "HSI16 oscillator did not become ready");
    E(ERR_RCC_HSE_TIMEOUT,      "HSE oscillator did not start (crystal/bypass?)");
    E(ERR_RCC_HSI48_TIMEOUT,    "HSI48 oscillator did not become ready");
    E(ERR_RCC_LSI_TIMEOUT,      "LSI oscillator did not become ready");
    E(ERR_RCC_LSE_TIMEOUT,      "LSE 32.768 kHz crystal did not start");
    E(ERR_RCC_PLL_TIMEOUT,      "PLL did not lock");
    E(ERR_RCC_SWITCH_TIMEOUT,   "SYSCLK source switch did not complete");
    E(ERR_RCC_FLASH_LATENCY,    "flash wait states not applied");
    E(ERR_RCC_VOS_TIMEOUT,      "voltage regulator did not settle");
    E(ERR_RCC_OSC_IN_USE,       "oscillator drives SYSCLK or the PLL");
    E(ERR_RCC_HSE_FREQ,         "HSE frequency outside 4..48 MHz");
    E(ERR_RCC_BAD_SOURCE,       "invalid clock source");
    E(ERR_RCC_BAD_PRESCALER,    "invalid AHB/APB prescaler");
    E(ERR_RCC_PLL_PARAM,        "PLL M/N/R/Q/P out of range");
    E(ERR_RCC_PLL_INPUT_RANGE,  "PLL input (src/M) outside 2.66..8 MHz");
    E(ERR_RCC_PLL_VCO_RANGE,    "PLL VCO outside 96..344 MHz");
    E(ERR_RCC_PLL_OUTPUT_RANGE, "PLL output above 170 MHz");
    E(ERR_RCC_HCLK_TOO_HIGH,    "HCLK above 170 MHz");

    /* GPIO */
    E(ERR_GPIO_NO_PINS,         "pin mask is 0");
    E(ERR_GPIO_BAD_MODE,        "invalid GPIO mode");
    E(ERR_GPIO_BAD_PULL,        "invalid pull configuration");
    E(ERR_GPIO_BAD_OTYPE,       "invalid output type");
    E(ERR_GPIO_BAD_SPEED,       "invalid output speed");
    E(ERR_GPIO_BAD_AF,          "alternate function above 15");
    E(ERR_GPIO_LOCK_FAILED,     "GPIO lock key sequence failed");

    /* UART */
    E(ERR_UART_BAD_INSTANCE,    "not a USART/UART/LPUART instance");
    E(ERR_UART_BAD_BAUD,        "baud rate not reachable from the kernel clock");
    E(ERR_UART_BAD_FORMAT,      "invalid data bits / parity / stop bits");
    E(ERR_UART_BAD_CLOCK,       "UART kernel clock source not running");
    E(ERR_UART_NOT_ENABLED,     "UART not initialized");
    E(ERR_UART_TX_TIMEOUT,      "transmit timed out");
    E(ERR_UART_RX_TIMEOUT,      "receive timed out");
    E(ERR_UART_RX_EMPTY,        "no byte received yet");
    E(ERR_UART_OVERRUN,         "RX overrun: byte lost, read faster");
    E(ERR_UART_FRAMING,         "framing error: baud mismatch or line break");
    E(ERR_UART_PARITY,          "parity error");
    E(ERR_UART_NOISE,           "noise detected on RX line");
    }
    return (err_info_t){ "ERR_UNKNOWN", "unknown error code" };
}

const char *err_name(err_t err){
    return lookup(err).name;
}

const char *err_desc(err_t err){
    return lookup(err).desc;
}

const char *err_module(err_t err){
    switch(((uint32_t)err >> 8) & 0xFFU){
    case 0x00: return "COMMON";
    case 0x01: return "RCC";
    case 0x02: return "GPIO";
    case 0x03: return "UART";
    default:   return "UNKNOWN";
    }
}
