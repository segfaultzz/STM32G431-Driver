#ifndef HARDWARE_RCC_H
#define HARDWARE_RCC_H

#include "hardware/err.h"
#include "stm32g4xx.h"
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 *  Peripheral identifiers
 * ============================================================================
 * Each ID packs the bus register index (bits 15..8) and the bit position
 * (bits 7..0). The bit position is identical in the ENR, RSTR and SMENR
 * registers of a bus, so one ID drives enable, reset and sleep-mode gating.
 *
 * Bus index = word offset from RCC->AHB1ENR (and from AHB1RSTR / AHB1SMENR).
 */
#define RCC_BUS_AHB1   0U
#define RCC_BUS_AHB2   1U
#define RCC_BUS_AHB3   2U /* no peripherals on STM32G431 */
#define RCC_BUS_APB1_1 4U
#define RCC_BUS_APB1_2 5U
#define RCC_BUS_APB2   6U

#define RCC_PERIPH_ID(bus, pos) (((uint32_t)(bus) << 8) | (uint32_t)(pos))

typedef enum {
  /* AHB1 */
  RCC_PERIPH_DMA1    = RCC_PERIPH_ID(RCC_BUS_AHB1, RCC_AHB1ENR_DMA1EN_Pos),
  RCC_PERIPH_DMA2    = RCC_PERIPH_ID(RCC_BUS_AHB1, RCC_AHB1ENR_DMA2EN_Pos),
  RCC_PERIPH_DMAMUX1 = RCC_PERIPH_ID(RCC_BUS_AHB1, RCC_AHB1ENR_DMAMUX1EN_Pos),
  RCC_PERIPH_CORDIC  = RCC_PERIPH_ID(RCC_BUS_AHB1, RCC_AHB1ENR_CORDICEN_Pos),
  RCC_PERIPH_FMAC    = RCC_PERIPH_ID(RCC_BUS_AHB1, RCC_AHB1ENR_FMACEN_Pos),
  RCC_PERIPH_FLASH   = RCC_PERIPH_ID(RCC_BUS_AHB1, RCC_AHB1ENR_FLASHEN_Pos),
  RCC_PERIPH_CRC     = RCC_PERIPH_ID(RCC_BUS_AHB1, RCC_AHB1ENR_CRCEN_Pos),

  /* AHB2 */
  RCC_PERIPH_GPIOA = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_GPIOAEN_Pos),
  RCC_PERIPH_GPIOB = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_GPIOBEN_Pos),
  RCC_PERIPH_GPIOC = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_GPIOCEN_Pos),
  RCC_PERIPH_GPIOD = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_GPIODEN_Pos),
  RCC_PERIPH_GPIOE = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_GPIOEEN_Pos),
  RCC_PERIPH_GPIOF = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_GPIOFEN_Pos),
  RCC_PERIPH_GPIOG = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_GPIOGEN_Pos),
  RCC_PERIPH_ADC12 = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_ADC12EN_Pos),
  RCC_PERIPH_DAC1  = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_DAC1EN_Pos),
  RCC_PERIPH_DAC3  = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_DAC3EN_Pos),
  RCC_PERIPH_RNG   = RCC_PERIPH_ID(RCC_BUS_AHB2, RCC_AHB2ENR_RNGEN_Pos),

  /* APB1 (register 1) */
  RCC_PERIPH_TIM2   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_TIM2EN_Pos),
  RCC_PERIPH_TIM3   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_TIM3EN_Pos),
  RCC_PERIPH_TIM4   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_TIM4EN_Pos),
  RCC_PERIPH_TIM6   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_TIM6EN_Pos),
  RCC_PERIPH_TIM7   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_TIM7EN_Pos),
  RCC_PERIPH_CRS    = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_CRSEN_Pos),
  RCC_PERIPH_RTCAPB = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_RTCAPBEN_Pos), /* no reset bit */
  RCC_PERIPH_WWDG   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_WWDGEN_Pos),   /* no reset bit */
  RCC_PERIPH_SPI2   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_SPI2EN_Pos),
  RCC_PERIPH_SPI3   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_SPI3EN_Pos),
  RCC_PERIPH_USART2 = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_USART2EN_Pos),
  RCC_PERIPH_USART3 = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_USART3EN_Pos),
  RCC_PERIPH_UART4  = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_UART4EN_Pos),
  RCC_PERIPH_I2C1   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_I2C1EN_Pos),
  RCC_PERIPH_I2C2   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_I2C2EN_Pos),
  RCC_PERIPH_USB    = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_USBEN_Pos),
  RCC_PERIPH_FDCAN  = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_FDCANEN_Pos),
  RCC_PERIPH_PWR    = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_PWREN_Pos),
  RCC_PERIPH_I2C3   = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_I2C3EN_Pos),
  RCC_PERIPH_LPTIM1 = RCC_PERIPH_ID(RCC_BUS_APB1_1, RCC_APB1ENR1_LPTIM1EN_Pos),

  /* APB1 (register 2) */
  RCC_PERIPH_LPUART1 = RCC_PERIPH_ID(RCC_BUS_APB1_2, RCC_APB1ENR2_LPUART1EN_Pos),
  RCC_PERIPH_UCPD1   = RCC_PERIPH_ID(RCC_BUS_APB1_2, RCC_APB1ENR2_UCPD1EN_Pos),

  /* APB2 */
  RCC_PERIPH_SYSCFG = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_SYSCFGEN_Pos),
  RCC_PERIPH_TIM1   = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_TIM1EN_Pos),
  RCC_PERIPH_SPI1   = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_SPI1EN_Pos),
  RCC_PERIPH_TIM8   = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_TIM8EN_Pos),
  RCC_PERIPH_USART1 = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_USART1EN_Pos),
  RCC_PERIPH_TIM15  = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_TIM15EN_Pos),
  RCC_PERIPH_TIM16  = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_TIM16EN_Pos),
  RCC_PERIPH_TIM17  = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_TIM17EN_Pos),
  RCC_PERIPH_SAI1   = RCC_PERIPH_ID(RCC_BUS_APB2, RCC_APB2ENR_SAI1EN_Pos),
} rcc_periph_t;

/* ============================================================================
 *  Oscillators
 * ============================================================================ */
typedef enum {
  RCC_OSC_HSI16, /* 16 MHz internal RC */
  RCC_OSC_HSE,   /* external crystal / clock (see rcc_clock_config_t.hse_hz) */
  RCC_OSC_HSI48, /* 48 MHz internal RC (USB, RNG) */
  RCC_OSC_LSI,   /* ~32 kHz internal RC (IWDG, RTC) */
  RCC_OSC_LSE,   /* 32.768 kHz external crystal (RTC) */
} rcc_osc_t;

/* ============================================================================
 *  System clock configuration
 * ============================================================================ */
typedef enum {
  RCC_SYSCLK_HSI16,
  RCC_SYSCLK_HSE,
  RCC_SYSCLK_PLL,
} rcc_sysclk_src_t;

typedef enum {
  RCC_PLLSRC_HSI16,
  RCC_PLLSRC_HSE,
} rcc_pll_src_t;

/* Values are the raw CFGR.HPRE encodings */
typedef enum {
  RCC_AHB_DIV1   = 0x0,
  RCC_AHB_DIV2   = 0x8,
  RCC_AHB_DIV4   = 0x9,
  RCC_AHB_DIV8   = 0xA,
  RCC_AHB_DIV16  = 0xB,
  RCC_AHB_DIV64  = 0xC,
  RCC_AHB_DIV128 = 0xD,
  RCC_AHB_DIV256 = 0xE,
  RCC_AHB_DIV512 = 0xF,
} rcc_ahb_div_t;

/* Values are the raw CFGR.PPREx encodings */
typedef enum {
  RCC_APB_DIV1  = 0x0,
  RCC_APB_DIV2  = 0x4,
  RCC_APB_DIV4  = 0x5,
  RCC_APB_DIV8  = 0x6,
  RCC_APB_DIV16 = 0x7,
} rcc_apb_div_t;

/*
 * PLL: f_vco = f_src / m * n,  PLLCLK (SYSCLK) = f_vco / r
 *   m     1..16,   f_src / m must be 2.66..8 MHz
 *   n     8..127,  f_vco must be 96..344 MHz
 *   r     2, 4, 6 or 8          (PLLR -> SYSCLK, max 170 MHz)
 *   q     0 = off, 2, 4, 6, 8   (PLLQ -> FDCAN, USB, QSPI...)
 *   p     0 = off, 2..31        (PLLP -> ADC, SAI)
 */
typedef struct {
  rcc_pll_src_t src;
  uint8_t m;
  uint8_t n;
  uint8_t r;
  uint8_t q;
  uint8_t p;
} rcc_pll_config_t;

typedef struct {
  rcc_sysclk_src_t sysclk_src;
  uint32_t hse_hz;  /* HSE frequency (4..48 MHz); ignored if HSE is unused */
  bool hse_bypass;  /* true: external clock on OSC_IN instead of a crystal */
  rcc_pll_config_t pll; /* ignored unless sysclk_src == RCC_SYSCLK_PLL */
  rcc_ahb_div_t ahb_div;
  rcc_apb_div_t apb1_div;
  rcc_apb_div_t apb2_div;
} rcc_clock_config_t;

/* Ready-made configurations */
extern const rcc_clock_config_t rcc_config_hsi16_16mhz;     /* reset state       */
extern const rcc_clock_config_t rcc_config_hsi16_170mhz;    /* HSI16/4*85/2      */
extern const rcc_clock_config_t rcc_config_hse24mhz_170mhz; /* 24MHz HSE/6*85/2  */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configures the system clock tree (SYSCLK, HCLK, PCLK1, PCLK2).
 *
 * Handles flash wait states, the Range 1 boost mode (> 150 MHz) and the AHB /2
 * transition step required when switching above 80 MHz (RM0440 6.1.5, 7.2.7).
 * Updates SystemCoreClock. The PLL is switched off when SYSCLK isn't the PLL.
 * Prefetch and instruction/data caches are enabled.
 *
 * @param config Desired configuration (e.g. &rcc_config_hsi16_170mhz).
 * @return ERR_OK, ERR_NULL_PTR,
 *         a validation error (ERR_RCC_BAD_PRESCALER, ERR_RCC_HSE_FREQ,
 *         ERR_RCC_BAD_SOURCE, ERR_RCC_PLL_PARAM, ERR_RCC_PLL_INPUT_RANGE,
 *         ERR_RCC_PLL_VCO_RANGE, ERR_RCC_PLL_OUTPUT_RANGE, ERR_RCC_HCLK_TOO_HIGH)
 *         returned before any register is touched, or a hardware timeout
 *         (ERR_RCC_HSE_TIMEOUT, ERR_RCC_PLL_TIMEOUT, ERR_RCC_SWITCH_TIMEOUT,
 *         ERR_RCC_FLASH_LATENCY, ERR_RCC_VOS_TIMEOUT...), after which the
 *         system is left running from HSI16.
 *
 * @warning Call before vTaskStartScheduler(): FreeRTOS programs SysTick from
 *          SystemCoreClock at scheduler start, and baud rates / timer settings
 *          computed earlier become wrong after a clock change.
 */
err_t rcc_clock_config(const rcc_clock_config_t *config);

/**
 * @brief Turns an oscillator on and waits until it is ready.
 * @return ERR_OK or ERR_RCC_<osc>_TIMEOUT (e.g. ERR_RCC_HSE_TIMEOUT).
 */
err_t rcc_osc_on(rcc_osc_t osc);

/**
 * @brief Turns an oscillator off.
 * @return ERR_RCC_OSC_IN_USE if it currently drives SYSCLK or the running PLL.
 */
err_t rcc_osc_off(rcc_osc_t osc);

/** @brief Returns true if the oscillator is on and stable. */
bool rcc_osc_is_ready(rcc_osc_t osc);

/* Current clock frequencies in Hz, computed from the RCC registers */
uint32_t rcc_get_sysclk_hz(void);
uint32_t rcc_get_hclk_hz(void);
uint32_t rcc_get_pclk1_hz(void);
uint32_t rcc_get_pclk2_hz(void);
/** @brief Timer kernel clock on APB1/APB2 (2 x PCLK when the APB prescaler != 1). */
uint32_t rcc_get_tim_apb1_hz(void);
uint32_t rcc_get_tim_apb2_hz(void);
/** @brief PLL Q / P outputs, 0 if the PLL or that output is off. */
uint32_t rcc_get_pllq_hz(void);
uint32_t rcc_get_pllp_hz(void);

/**
 * @brief Enables the bus clock of a peripheral. Must be done before touching
 *        any of its registers. Safe to call from tasks and ISRs.
 * @return ERR_OK, or ERR_RCC_UNKNOWN_PERIPH for an unknown ID.
 */
err_t rcc_periph_clock_enable(rcc_periph_t periph);

/** @brief Disables the bus clock of a peripheral (its registers keep their values). */
err_t rcc_periph_clock_disable(rcc_periph_t periph);

/** @brief Returns true if the peripheral's bus clock is enabled. */
bool rcc_periph_clock_is_enabled(rcc_periph_t periph);

/**
 * @brief Pulses the peripheral's reset line: all its registers go back to
 *        reset values. The clock enable state is not changed.
 * @return ERR_RCC_NO_RESET_BIT for RTCAPB and WWDG.
 */
err_t rcc_periph_reset(rcc_periph_t periph);

/**
 * @brief Chooses whether the peripheral keeps its clock in Sleep mode
 *        (SMENR registers, all enabled after reset).
 */
err_t rcc_periph_sleep_clock(rcc_periph_t periph, bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_RCC_H */
