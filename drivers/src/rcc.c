#include "hardware/rcc.h"
#include <stddef.h>

/* ---------------------------------------------------------------------------
 *  Limits (RM0440 / DS12589)
 * ------------------------------------------------------------------------- */
#define HSI16_HZ          16000000U
#define HSI48_HZ          48000000U
#define HSE_DEFAULT_HZ    24000000U   /* matches HSE_VALUE in system_stm32g4xx.c */
#define HSE_MIN_HZ         4000000U
#define HSE_MAX_HZ        48000000U
#define PLL_IN_MIN_HZ      2660000U
#define PLL_IN_MAX_HZ      8000000U
#define VCO_MIN_HZ        96000000U
#define VCO_MAX_HZ       344000000U
#define PLL_OUT_MAX_HZ   170000000U
#define HCLK_MAX_HZ      170000000U
#define BOOST_ABOVE_HZ   150000000U   /* Range 1 normal mode max */
#define AHB_STEP_ABOVE_HZ 80000000U   /* switch with AHB /2 first above this */
#define WS_STEP_BOOST_HZ  34000000U   /* Range 1 boost: one wait state per 34 MHz */
#define WS_STEP_NORMAL_HZ 30000000U   /* Range 1 normal: one wait state per 30 MHz */
#define FLASH_MAX_WS      FLASH_ACR_LATENCY_4WS

#define TIMEOUT_LOOPS      1000000U
#define LSE_TIMEOUT_LOOPS 50000000U   /* LSE crystals can take ~2 s to start */

/* HSE frequency in use; HSE_VALUE is private to system_stm32g4xx.c */
static uint32_t s_hse_hz = HSE_DEFAULT_HZ;

/* ---------------------------------------------------------------------------
 *  Presets
 * ------------------------------------------------------------------------- */
const rcc_clock_config_t rcc_config_hsi16_16mhz = {
    .sysclk_src = RCC_SYSCLK_HSI16,
    .ahb_div = RCC_AHB_DIV1, .apb1_div = RCC_APB_DIV1, .apb2_div = RCC_APB_DIV1,
};

/* 16 MHz / 4 = 4 MHz, * 85 = 340 MHz VCO, / 2 = 170 MHz */
const rcc_clock_config_t rcc_config_hsi16_170mhz = {
    .sysclk_src = RCC_SYSCLK_PLL,
    .pll = { .src = RCC_PLLSRC_HSI16, .m = 4, .n = 85, .r = 2, .q = 0, .p = 0 },
    .ahb_div = RCC_AHB_DIV1, .apb1_div = RCC_APB_DIV1, .apb2_div = RCC_APB_DIV1,
};

/* 24 MHz / 6 = 4 MHz, * 85 = 340 MHz VCO, / 2 = 170 MHz */
const rcc_clock_config_t rcc_config_hse24mhz_170mhz = {
    .sysclk_src = RCC_SYSCLK_PLL,
    .hse_hz = 24000000U,
    .hse_bypass = false,
    .pll = { .src = RCC_PLLSRC_HSE, .m = 6, .n = 85, .r = 2, .q = 0, .p = 0 },
    .ahb_div = RCC_AHB_DIV1, .apb1_div = RCC_APB_DIV1, .apb2_div = RCC_APB_DIV1,
};

/* ---------------------------------------------------------------------------
 *  Peripheral clock gating
 * ------------------------------------------------------------------------- */

/* The bus index is a word offset from AHB1ENR / AHB1RSTR / AHB1SMENR.
 * Check the register map really is laid out that way. */
_Static_assert(offsetof(RCC_TypeDef, AHB2ENR)  - offsetof(RCC_TypeDef, AHB1ENR) == 4U * RCC_BUS_AHB2,   "RCC layout");
_Static_assert(offsetof(RCC_TypeDef, APB1ENR1) - offsetof(RCC_TypeDef, AHB1ENR) == 4U * RCC_BUS_APB1_1, "RCC layout");
_Static_assert(offsetof(RCC_TypeDef, APB1ENR2) - offsetof(RCC_TypeDef, AHB1ENR) == 4U * RCC_BUS_APB1_2, "RCC layout");
_Static_assert(offsetof(RCC_TypeDef, APB2ENR)  - offsetof(RCC_TypeDef, AHB1ENR) == 4U * RCC_BUS_APB2,   "RCC layout");
_Static_assert(offsetof(RCC_TypeDef, APB2RSTR) - offsetof(RCC_TypeDef, AHB1RSTR) == 4U * RCC_BUS_APB2,  "RCC layout");
_Static_assert(offsetof(RCC_TypeDef, APB2SMENR) - offsetof(RCC_TypeDef, AHB1SMENR) == 4U * RCC_BUS_APB2, "RCC layout");

#define BUS_COUNT 7U

/* Bits that exist in each ENR register (index = bus) */
static const uint32_t enr_valid[BUS_COUNT] = {
    [RCC_BUS_AHB1] = RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_DMAMUX1EN |
                     RCC_AHB1ENR_CORDICEN | RCC_AHB1ENR_FMACEN | RCC_AHB1ENR_FLASHEN |
                     RCC_AHB1ENR_CRCEN,
    [RCC_BUS_AHB2] = RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN |
                     RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_GPIOEEN | RCC_AHB2ENR_GPIOFEN |
                     RCC_AHB2ENR_GPIOGEN | RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_DAC1EN |
                     RCC_AHB2ENR_DAC3EN | RCC_AHB2ENR_RNGEN,
    [RCC_BUS_APB1_1] = RCC_APB1ENR1_TIM2EN | RCC_APB1ENR1_TIM3EN | RCC_APB1ENR1_TIM4EN |
                       RCC_APB1ENR1_TIM6EN | RCC_APB1ENR1_TIM7EN | RCC_APB1ENR1_CRSEN |
                       RCC_APB1ENR1_RTCAPBEN | RCC_APB1ENR1_WWDGEN | RCC_APB1ENR1_SPI2EN |
                       RCC_APB1ENR1_SPI3EN | RCC_APB1ENR1_USART2EN | RCC_APB1ENR1_USART3EN |
                       RCC_APB1ENR1_UART4EN | RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_I2C2EN |
                       RCC_APB1ENR1_USBEN | RCC_APB1ENR1_FDCANEN | RCC_APB1ENR1_PWREN |
                       RCC_APB1ENR1_I2C3EN | RCC_APB1ENR1_LPTIM1EN,
    [RCC_BUS_APB1_2] = RCC_APB1ENR2_LPUART1EN | RCC_APB1ENR2_UCPD1EN,
    [RCC_BUS_APB2] = RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_SPI1EN |
                     RCC_APB2ENR_TIM8EN | RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM15EN |
                     RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN | RCC_APB2ENR_SAI1EN,
};

/* Splits an ID into register index and bit mask. False if the ID is unknown. */
static bool decode(rcc_periph_t periph, uint32_t *bus, uint32_t *mask){
    uint32_t b = (uint32_t)periph >> 8;
    uint32_t pos = (uint32_t)periph & 0xFFU;
    if(b >= BUS_COUNT || pos > 31U){
        return false;
    }
    uint32_t m = 1UL << pos;
    if((enr_valid[b] & m) == 0U){
        return false;
    }
    *bus = b;
    *mask = m;
    return true;
}

/* Read-modify-write of a shared RCC register, safe against tasks and ISRs */
static void rmw(volatile uint32_t *reg, uint32_t clear, uint32_t set){
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    *reg = (*reg & ~clear) | set;
    __set_PRIMASK(primask);
}

err_t rcc_periph_clock_enable(rcc_periph_t periph){
    uint32_t bus, mask;
    if(!decode(periph, &bus, &mask)){
        return ERR_RCC_UNKNOWN_PERIPH;
    }
    volatile uint32_t *enr = &RCC->AHB1ENR + bus;
    rmw(enr, 0U, mask);
    (void)*enr;   /* read back: the clock takes 2 cycles to reach the peripheral */
    return ERR_OK;
}

err_t rcc_periph_clock_disable(rcc_periph_t periph){
    uint32_t bus, mask;
    if(!decode(periph, &bus, &mask)){
        return ERR_RCC_UNKNOWN_PERIPH;
    }
    rmw(&RCC->AHB1ENR + bus, mask, 0U);
    return ERR_OK;
}

bool rcc_periph_clock_is_enabled(rcc_periph_t periph){
    uint32_t bus, mask;
    if(!decode(periph, &bus, &mask)){
        return false;
    }
    return ((&RCC->AHB1ENR)[bus] & mask) != 0U;
}

err_t rcc_periph_reset(rcc_periph_t periph){
    uint32_t bus, mask;
    if(!decode(periph, &bus, &mask)){
        return ERR_RCC_UNKNOWN_PERIPH;
    }
    if(periph == RCC_PERIPH_RTCAPB || periph == RCC_PERIPH_WWDG){
        return ERR_RCC_NO_RESET_BIT;   /* no bit in APB1RSTR1 */
    }
    volatile uint32_t *rstr = &RCC->AHB1RSTR + bus;
    rmw(rstr, 0U, mask);
    (void)*rstr;
    rmw(rstr, mask, 0U);
    return ERR_OK;
}

err_t rcc_periph_sleep_clock(rcc_periph_t periph, bool enabled){
    uint32_t bus, mask;
    if(!decode(periph, &bus, &mask)){
        return ERR_RCC_UNKNOWN_PERIPH;
    }
    rmw(&RCC->AHB1SMENR + bus, enabled ? 0U : mask, enabled ? mask : 0U);
    return ERR_OK;
}

/* ---------------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------------- */
static err_t wait_bits(volatile uint32_t *reg, uint32_t mask, uint32_t expected,
                       uint32_t loops, err_t on_timeout){
    while((*reg & mask) != expected){
        if(loops-- == 0U){
            return on_timeout;
        }
    }
    return ERR_OK;
}

static void delay_loops(uint32_t loops){
    while(loops--){
        __NOP();
    }
}

static uint32_t ahb_divider(uint32_t hpre){
    static const uint16_t div[8] = {2, 4, 8, 16, 64, 128, 256, 512};
    return (hpre < 8U) ? 1U : div[hpre - 8U];
}

static uint32_t apb_divider(uint32_t ppre){
    return (ppre < 4U) ? 1U : (1U << (ppre - 3U));
}

static bool ahb_div_valid(rcc_ahb_div_t d){
    return d == RCC_AHB_DIV1 || (d >= RCC_AHB_DIV2 && d <= RCC_AHB_DIV512);
}

static bool apb_div_valid(rcc_apb_div_t d){
    return d == RCC_APB_DIV1 || (d >= RCC_APB_DIV2 && d <= RCC_APB_DIV16);
}

static uint32_t sysclk_switch_status(void){
    return RCC->CFGR & RCC_CFGR_SWS;
}

static bool pll_running_from(uint32_t pllsrc){
    return (RCC->CR & RCC_CR_PLLON) && (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) == pllsrc;
}

static err_t set_flash_latency(uint32_t ws){
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | ws;
    /* The new value must be read back before the clock is changed */
    return wait_bits(&FLASH->ACR, FLASH_ACR_LATENCY, ws, TIMEOUT_LOOPS, ERR_RCC_FLASH_LATENCY);
}

/* Enables PWR's clock and gives write access to the backup domain (LSE, RTC) */
static void backup_domain_unlock(void){
    (void)rcc_periph_clock_enable(RCC_PERIPH_PWR);
    PWR->CR1 |= PWR_CR1_DBP;
    (void)PWR->CR1;
}

/* ---------------------------------------------------------------------------
 *  Oscillators
 * ------------------------------------------------------------------------- */
err_t rcc_osc_on(rcc_osc_t osc){
    switch(osc){
    case RCC_OSC_HSI16:
        RCC->CR |= RCC_CR_HSION;
        return wait_bits(&RCC->CR, RCC_CR_HSIRDY, RCC_CR_HSIRDY, TIMEOUT_LOOPS, ERR_RCC_HSI16_TIMEOUT);
    case RCC_OSC_HSE:
        RCC->CR |= RCC_CR_HSEON;
        return wait_bits(&RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY, TIMEOUT_LOOPS, ERR_RCC_HSE_TIMEOUT);
    case RCC_OSC_HSI48:
        RCC->CRRCR |= RCC_CRRCR_HSI48ON;
        return wait_bits(&RCC->CRRCR, RCC_CRRCR_HSI48RDY, RCC_CRRCR_HSI48RDY, TIMEOUT_LOOPS, ERR_RCC_HSI48_TIMEOUT);
    case RCC_OSC_LSI:
        RCC->CSR |= RCC_CSR_LSION;
        return wait_bits(&RCC->CSR, RCC_CSR_LSIRDY, RCC_CSR_LSIRDY, TIMEOUT_LOOPS, ERR_RCC_LSI_TIMEOUT);
    case RCC_OSC_LSE:
        backup_domain_unlock();
        RCC->BDCR |= RCC_BDCR_LSEON;
        return wait_bits(&RCC->BDCR, RCC_BDCR_LSERDY, RCC_BDCR_LSERDY, LSE_TIMEOUT_LOOPS, ERR_RCC_LSE_TIMEOUT);
    default:
        return ERR_INVALID_PARAM;
    }
}

err_t rcc_osc_off(rcc_osc_t osc){
    switch(osc){
    case RCC_OSC_HSI16:
        if(sysclk_switch_status() == RCC_CFGR_SWS_HSI || pll_running_from(RCC_PLLCFGR_PLLSRC_HSI)){
            return ERR_RCC_OSC_IN_USE;
        }
        RCC->CR &= ~RCC_CR_HSION;
        return wait_bits(&RCC->CR, RCC_CR_HSIRDY, 0U, TIMEOUT_LOOPS, ERR_RCC_HSI16_TIMEOUT);
    case RCC_OSC_HSE:
        if(sysclk_switch_status() == RCC_CFGR_SWS_HSE || pll_running_from(RCC_PLLCFGR_PLLSRC_HSE)){
            return ERR_RCC_OSC_IN_USE;
        }
        RCC->CR &= ~RCC_CR_HSEON;
        return wait_bits(&RCC->CR, RCC_CR_HSERDY, 0U, TIMEOUT_LOOPS, ERR_RCC_HSE_TIMEOUT);
    case RCC_OSC_HSI48:
        RCC->CRRCR &= ~RCC_CRRCR_HSI48ON;
        return wait_bits(&RCC->CRRCR, RCC_CRRCR_HSI48RDY, 0U, TIMEOUT_LOOPS, ERR_RCC_HSI48_TIMEOUT);
    case RCC_OSC_LSI:
        RCC->CSR &= ~RCC_CSR_LSION;
        return wait_bits(&RCC->CSR, RCC_CSR_LSIRDY, 0U, TIMEOUT_LOOPS, ERR_RCC_LSI_TIMEOUT);
    case RCC_OSC_LSE:
        backup_domain_unlock();
        RCC->BDCR &= ~RCC_BDCR_LSEON;
        return wait_bits(&RCC->BDCR, RCC_BDCR_LSERDY, 0U, TIMEOUT_LOOPS, ERR_RCC_LSE_TIMEOUT);
    default:
        return ERR_INVALID_PARAM;
    }
}

bool rcc_osc_is_ready(rcc_osc_t osc){
    switch(osc){
    case RCC_OSC_HSI16: return (RCC->CR & RCC_CR_HSIRDY) != 0U;
    case RCC_OSC_HSE:   return (RCC->CR & RCC_CR_HSERDY) != 0U;
    case RCC_OSC_HSI48: return (RCC->CRRCR & RCC_CRRCR_HSI48RDY) != 0U;
    case RCC_OSC_LSI:   return (RCC->CSR & RCC_CSR_LSIRDY) != 0U;
    case RCC_OSC_LSE:   return (RCC->BDCR & RCC_BDCR_LSERDY) != 0U;
    default:            return false;
    }
}

/* ---------------------------------------------------------------------------
 *  System clock
 * ------------------------------------------------------------------------- */

/* Checks the configuration against the datasheet limits and returns SYSCLK */
static err_t validate(const rcc_clock_config_t *c, uint32_t *sysclk_out){
    if(!ahb_div_valid(c->ahb_div) || !apb_div_valid(c->apb1_div) || !apb_div_valid(c->apb2_div)){
        return ERR_RCC_BAD_PRESCALER;
    }
    bool uses_hse = c->sysclk_src == RCC_SYSCLK_HSE ||
                    (c->sysclk_src == RCC_SYSCLK_PLL && c->pll.src == RCC_PLLSRC_HSE);
    if(uses_hse && (c->hse_hz < HSE_MIN_HZ || c->hse_hz > HSE_MAX_HZ)){
        return ERR_RCC_HSE_FREQ;
    }

    uint32_t sysclk;
    switch(c->sysclk_src){
    case RCC_SYSCLK_HSI16:
        sysclk = HSI16_HZ;
        break;
    case RCC_SYSCLK_HSE:
        sysclk = c->hse_hz;
        break;
    case RCC_SYSCLK_PLL: {
        const rcc_pll_config_t *p = &c->pll;
        if(p->src != RCC_PLLSRC_HSI16 && p->src != RCC_PLLSRC_HSE){
            return ERR_RCC_BAD_SOURCE;
        }
        if(p->m < 1U || p->m > 16U){
            return ERR_RCC_PLL_PARAM;
        }
        if(p->n < 8U || p->n > 127U){
            return ERR_RCC_PLL_PARAM;
        }
        if(p->r < 2U || p->r > 8U || (p->r & 1U)){
            return ERR_RCC_PLL_PARAM;
        }
        if(p->q != 0U && (p->q < 2U || p->q > 8U || (p->q & 1U))){
            return ERR_RCC_PLL_PARAM;
        }
        if(p->p != 0U && (p->p < 2U || p->p > 31U)){
            return ERR_RCC_PLL_PARAM;
        }

        uint32_t in = ((p->src == RCC_PLLSRC_HSE) ? c->hse_hz : HSI16_HZ) / p->m;
        if(in < PLL_IN_MIN_HZ || in > PLL_IN_MAX_HZ){
            return ERR_RCC_PLL_INPUT_RANGE;
        }
        uint32_t vco = in * p->n;
        if(vco < VCO_MIN_HZ || vco > VCO_MAX_HZ){
            return ERR_RCC_PLL_VCO_RANGE;
        }
        if(vco / p->r > PLL_OUT_MAX_HZ){
            return ERR_RCC_PLL_OUTPUT_RANGE;
        }
        if(p->q != 0U && vco / p->q > PLL_OUT_MAX_HZ){
            return ERR_RCC_PLL_OUTPUT_RANGE;
        }
        if(p->p != 0U && vco / p->p > PLL_OUT_MAX_HZ){
            return ERR_RCC_PLL_OUTPUT_RANGE;
        }
        sysclk = vco / p->r;
        break;
    }
    default:
        return ERR_RCC_BAD_SOURCE;
    }
    if(sysclk / ahb_divider((uint32_t)c->ahb_div) > HCLK_MAX_HZ){
        return ERR_RCC_HCLK_TOO_HIGH;
    }
    *sysclk_out = sysclk;
    return ERR_OK;
}

static uint32_t pllcfgr_value(const rcc_clock_config_t *c){
    const rcc_pll_config_t *p = &c->pll;
    uint32_t v = (p->src == RCC_PLLSRC_HSE) ? RCC_PLLCFGR_PLLSRC_HSE : RCC_PLLCFGR_PLLSRC_HSI;
    v |= (uint32_t)(p->m - 1U) << RCC_PLLCFGR_PLLM_Pos;
    v |= (uint32_t)p->n << RCC_PLLCFGR_PLLN_Pos;
    v |= (uint32_t)(p->r / 2U - 1U) << RCC_PLLCFGR_PLLR_Pos;   /* 00:/2 01:/4 10:/6 11:/8 */
    v |= RCC_PLLCFGR_PLLREN;
    if(p->q != 0U){
        v |= (uint32_t)(p->q / 2U - 1U) << RCC_PLLCFGR_PLLQ_Pos;
        v |= RCC_PLLCFGR_PLLQEN;
    }
    if(p->p != 0U){
        v |= (uint32_t)p->p << RCC_PLLCFGR_PLLPDIV_Pos;
        v |= RCC_PLLCFGR_PLLPEN;
    }
    return v;
}

static err_t pll_off(void){
    RCC->CR &= ~RCC_CR_PLLON;
    return wait_bits(&RCC->CR, RCC_CR_PLLRDY, 0U, TIMEOUT_LOOPS, ERR_RCC_PLL_TIMEOUT);
}

/* Error path of rcc_clock_config(): makes sure SYSCLK is HSI16 again (a failed
 * switch leaves SW pointing at the new source) and that SystemCoreClock
 * matches what is really running. Flash stays at the safe maximum latency. */
static err_t fallback_hsi(err_t err){
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    (void)wait_bits(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_HSI, TIMEOUT_LOOPS, ERR_RCC_SWITCH_TIMEOUT);
    SystemCoreClock = rcc_get_hclk_hz();
    return err;
}

err_t rcc_clock_config(const rcc_clock_config_t *config){
    if(config == NULL){
        return ERR_NULL_PTR;
    }
    uint32_t sysclk;
    err_t err = validate(config, &sysclk);
    if(err != ERR_OK){
        return err;
    }

    uint32_t hclk = sysclk / ahb_divider((uint32_t)config->ahb_div);
    bool boost = hclk > BOOST_ABOVE_HZ;
    bool uses_hse = config->sysclk_src == RCC_SYSCLK_HSE ||
                    (config->sysclk_src == RCC_SYSCLK_PLL && config->pll.src == RCC_PLLSRC_HSE);

    /* 1. Worst-case wait states: safe for every frequency used below */
    err = set_flash_latency(FLASH_MAX_WS);
    if(err != ERR_OK){
        return err;
    }

    /* 2. Fall back to HSI16 so the PLL and HSE can be reprogrammed */
    err = rcc_osc_on(RCC_OSC_HSI16);
    if(err != ERR_OK){
        return err;
    }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
    err = wait_bits(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_HSI, TIMEOUT_LOOPS, ERR_RCC_SWITCH_TIMEOUT);
    if(err != ERR_OK){
        return err;   /* still on the old clock: SystemCoreClock is still right */
    }
    SystemCoreClock = rcc_get_hclk_hz();   /* HSI16 with the old AHB prescaler */

    err = pll_off();
    if(err != ERR_OK){
        return fallback_hsi(err);
    }

    /* 3. HSE (bypass can only change while HSE is off) */
    if(uses_hse){
        uint32_t want_byp = config->hse_bypass ? RCC_CR_HSEBYP : 0U;
        if((RCC->CR & RCC_CR_HSEBYP) != want_byp){
            RCC->CR &= ~RCC_CR_HSEON;
            err = wait_bits(&RCC->CR, RCC_CR_HSERDY, 0U, TIMEOUT_LOOPS, ERR_RCC_HSE_TIMEOUT);
            if(err != ERR_OK){
                return fallback_hsi(err);
            }
            RCC->CR = (RCC->CR & ~RCC_CR_HSEBYP) | want_byp;
        }
        err = rcc_osc_on(RCC_OSC_HSE);
        if(err != ERR_OK){
            return fallback_hsi(err);
        }
        s_hse_hz = config->hse_hz;
    }

    /* 4. PLL */
    if(config->sysclk_src == RCC_SYSCLK_PLL){
        RCC->PLLCFGR = pllcfgr_value(config);
        RCC->CR |= RCC_CR_PLLON;
        err = wait_bits(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY, TIMEOUT_LOOPS, ERR_RCC_PLL_TIMEOUT);
        if(err != ERR_OK){
            (void)pll_off();
            return fallback_hsi(err);
        }
    }

    /* 5. Voltage range 1, boost mode only when needed (we're at 16 MHz now) */
    (void)rcc_periph_clock_enable(RCC_PERIPH_PWR);
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    err = wait_bits(&PWR->SR2, PWR_SR2_VOSF, 0U, TIMEOUT_LOOPS, ERR_RCC_VOS_TIMEOUT);
    if(err != ERR_OK){
        return fallback_hsi(err);
    }
    if(boost){
        PWR->CR5 &= ~PWR_CR5_R1MODE;
    } else {
        PWR->CR5 |= PWR_CR5_R1MODE;
    }

    /* 6. Prescalers. Above 80 MHz with AHB /1, go through AHB /2 for >= 1 us
     *    to limit the current step (RM0440 7.2.7). */
    bool ahb_step = sysclk > AHB_STEP_ABOVE_HZ && config->ahb_div == RCC_AHB_DIV1;
    uint32_t hpre = ahb_step ? (uint32_t)RCC_AHB_DIV2 : (uint32_t)config->ahb_div;
    uint32_t cfgr = RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    cfgr |= hpre << RCC_CFGR_HPRE_Pos;
    cfgr |= (uint32_t)config->apb1_div << RCC_CFGR_PPRE1_Pos;
    cfgr |= (uint32_t)config->apb2_div << RCC_CFGR_PPRE2_Pos;
    RCC->CFGR = cfgr;

    /* 7. Switch SYSCLK */
    uint32_t sw, sws;
    switch(config->sysclk_src){
    case RCC_SYSCLK_HSE: sw = RCC_CFGR_SW_HSE; sws = RCC_CFGR_SWS_HSE; break;
    case RCC_SYSCLK_PLL: sw = RCC_CFGR_SW_PLL; sws = RCC_CFGR_SWS_PLL; break;
    default:             sw = RCC_CFGR_SW_HSI; sws = RCC_CFGR_SWS_HSI; break;
    }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | sw;
    err = wait_bits(&RCC->CFGR, RCC_CFGR_SWS, sws, TIMEOUT_LOOPS, ERR_RCC_SWITCH_TIMEOUT);
    if(err != ERR_OK){
        return fallback_hsi(err);
    }

    if(ahb_step){
        delay_loops(200U);   /* >= 1 us at up to 170 MHz */
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_HPRE) | ((uint32_t)config->ahb_div << RCC_CFGR_HPRE_Pos);
    }
    SystemCoreClock = hclk;   /* new clock is live from here on */

    /* 8. Lowest wait states for the final HCLK, then prefetch + caches */
    uint32_t ws = (hclk - 1U) / (boost ? WS_STEP_BOOST_HZ : WS_STEP_NORMAL_HZ);
    err = set_flash_latency(ws);
    if(err != ERR_OK){
        return err;
    }
    FLASH->ACR |= FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    /* 9. PLL is only kept running when it drives SYSCLK */
    if(config->sysclk_src != RCC_SYSCLK_PLL){
        (void)pll_off();
    }

    return ERR_OK;
}

/* ---------------------------------------------------------------------------
 *  Frequency queries
 * ------------------------------------------------------------------------- */
static uint32_t pll_vco_hz(void){
    uint32_t cfg = RCC->PLLCFGR;
    uint32_t in;
    switch(cfg & RCC_PLLCFGR_PLLSRC){
    case RCC_PLLCFGR_PLLSRC_HSI: in = HSI16_HZ; break;
    case RCC_PLLCFGR_PLLSRC_HSE: in = s_hse_hz; break;
    default: return 0U;
    }
    uint32_t m = ((cfg & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1U;
    uint32_t n = (cfg & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos;
    return in / m * n;
}

uint32_t rcc_get_sysclk_hz(void){
    switch(sysclk_switch_status()){
    case RCC_CFGR_SWS_HSI: return HSI16_HZ;
    case RCC_CFGR_SWS_HSE: return s_hse_hz;
    case RCC_CFGR_SWS_PLL: {
        uint32_t r = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) * 2U + 2U;
        return pll_vco_hz() / r;
    }
    default: return 0U;
    }
}

uint32_t rcc_get_hclk_hz(void){
    uint32_t hpre = (RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos;
    return rcc_get_sysclk_hz() / ahb_divider(hpre);
}

uint32_t rcc_get_pclk1_hz(void){
    uint32_t ppre = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    return rcc_get_hclk_hz() / apb_divider(ppre);
}

uint32_t rcc_get_pclk2_hz(void){
    uint32_t ppre = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
    return rcc_get_hclk_hz() / apb_divider(ppre);
}

uint32_t rcc_get_tim_apb1_hz(void){
    uint32_t ppre = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    return (apb_divider(ppre) == 1U) ? rcc_get_pclk1_hz() : rcc_get_pclk1_hz() * 2U;
}

uint32_t rcc_get_tim_apb2_hz(void){
    uint32_t ppre = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
    return (apb_divider(ppre) == 1U) ? rcc_get_pclk2_hz() : rcc_get_pclk2_hz() * 2U;
}

uint32_t rcc_get_pllq_hz(void){
    uint32_t cfg = RCC->PLLCFGR;
    if(!(RCC->CR & RCC_CR_PLLRDY) || !(cfg & RCC_PLLCFGR_PLLQEN)){
        return 0U;
    }
    uint32_t q = ((cfg & RCC_PLLCFGR_PLLQ) >> RCC_PLLCFGR_PLLQ_Pos) * 2U + 2U;
    return pll_vco_hz() / q;
}

uint32_t rcc_get_pllp_hz(void){
    uint32_t cfg = RCC->PLLCFGR;
    if(!(RCC->CR & RCC_CR_PLLRDY) || !(cfg & RCC_PLLCFGR_PLLPEN)){
        return 0U;
    }
    uint32_t p = (cfg & RCC_PLLCFGR_PLLPDIV) >> RCC_PLLCFGR_PLLPDIV_Pos;
    if(p == 0U){   /* PLLPDIV unused: legacy PLLP bit selects /7 or /17 */
        p = (cfg & RCC_PLLCFGR_PLLP) ? 17U : 7U;
    }
    return pll_vco_hz() / p;
}
