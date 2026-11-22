#include "hardware/uart.h"
#include "hardware/rcc.h"
#include <stdbool.h>

#define LSE_HZ   32768U
#define HSI16_HZ 16000000U

/* ---------------------------------------------------------------------------
 *  Instance table
 * ------------------------------------------------------------------------- */
typedef struct {
    USART_TypeDef *regs;
    rcc_periph_t periph;
    uint8_t ccipr_pos;   /* position of the 2-bit xxxSEL field in RCC_CCIPR */
    bool on_apb2;        /* PCLK2 instead of PCLK1 */
    bool low_power;      /* LPUART: different baud formula */
} uart_hw_t;

static const uart_hw_t hw_table[] = {
    { USART1,  RCC_PERIPH_USART1,  RCC_CCIPR_USART1SEL_Pos,  true,  false },
    { USART2,  RCC_PERIPH_USART2,  RCC_CCIPR_USART2SEL_Pos,  false, false },
    { USART3,  RCC_PERIPH_USART3,  RCC_CCIPR_USART3SEL_Pos,  false, false },
    { UART4,   RCC_PERIPH_UART4,   RCC_CCIPR_UART4SEL_Pos,   false, false },
    { LPUART1, RCC_PERIPH_LPUART1, RCC_CCIPR_LPUART1SEL_Pos, false, true  },
};

static const uart_hw_t *find_hw(const USART_TypeDef *uart){
    for(size_t i = 0; i < sizeof hw_table / sizeof hw_table[0]; i++){
        if(hw_table[i].regs == uart){
            return &hw_table[i];
        }
    }
    return NULL;
}

/* Common argument check for the data-path functions */
static err_t check_ready(USART_TypeDef *uart){
    if(uart == NULL){
        return ERR_NULL_PTR;
    }
    if(find_hw(uart) == NULL){
        return ERR_UART_BAD_INSTANCE;
    }
    if(!(uart->CR1 & USART_CR1_UE)){
        return ERR_UART_NOT_ENABLED;
    }
    return ERR_OK;
}

/* ---------------------------------------------------------------------------
 *  Millisecond timeouts on the DWT cycle counter
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t last;
    uint64_t cycles_left;
    bool forever;
} timeout_t;

static void timeout_start(timeout_t *t, uint32_t ms){
    if(!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)){
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    t->forever = (ms == UART_WAIT_FOREVER);
    t->cycles_left = (uint64_t)(SystemCoreClock / 1000U) * ms;
    t->last = DWT->CYCCNT;
}

/* Accumulates elapsed cycles, so the 32-bit counter may wrap (~25 s at 170 MHz) */
static bool timeout_expired(timeout_t *t){
    if(t->forever){
        return false;
    }
    uint32_t now = DWT->CYCCNT;
    uint32_t elapsed = now - t->last;
    t->last = now;
    if(elapsed >= t->cycles_left){
        return true;
    }
    t->cycles_left -= elapsed;
    return false;
}

/* ---------------------------------------------------------------------------
 *  Setup
 * ------------------------------------------------------------------------- */
static err_t kernel_clock_hz(const uart_hw_t *hw, uart_clk_t clk, uint32_t *hz){
    switch(clk){
    case UART_CLK_PCLK:
        *hz = hw->on_apb2 ? rcc_get_pclk2_hz() : rcc_get_pclk1_hz();
        return ERR_OK;
    case UART_CLK_SYSCLK:
        *hz = rcc_get_sysclk_hz();
        return ERR_OK;
    case UART_CLK_HSI16:
        if(!rcc_osc_is_ready(RCC_OSC_HSI16)) return ERR_UART_BAD_CLOCK;
        *hz = HSI16_HZ;
        return ERR_OK;
    case UART_CLK_LSE:
        if(!rcc_osc_is_ready(RCC_OSC_LSE)) return ERR_UART_BAD_CLOCK;
        *hz = LSE_HZ;
        return ERR_OK;
    default:
        return ERR_UART_BAD_CLOCK;
    }
}

/* Baud divider (oversampling by 16 for USART, x256 formula for LPUART) */
static err_t baud_divider(const uart_hw_t *hw, uint32_t clk_hz, uint32_t baud, uint32_t *brr){
    if(baud == 0U){
        return ERR_UART_BAD_BAUD;
    }
    if(hw->low_power){
        /* RM0440: f_ck must be 3..4096 x baud, BRR >= 0x300, 20 bits */
        if((uint64_t)clk_hz < 3ULL * baud || (uint64_t)clk_hz > 4096ULL * baud){
            return ERR_UART_BAD_BAUD;
        }
        uint64_t div = ((uint64_t)clk_hz * 256U + baud / 2U) / baud;
        if(div < 0x300U || div > 0xFFFFFU){
            return ERR_UART_BAD_BAUD;
        }
        *brr = (uint32_t)div;
    } else {
        uint32_t div = (clk_hz + baud / 2U) / baud;
        if(div < 16U || div > 0xFFFFU){
            return ERR_UART_BAD_BAUD;
        }
        *brr = div;
    }
    return ERR_OK;
}

/* CR1 word length (M1:M0) and parity bits. The parity bit counts as a data bit */
static err_t frame_bits(const uart_config_t *c, uint32_t *cr1){
    if(c->parity > UART_PARITY_ODD || c->stop > UART_STOP_2){
        return ERR_UART_BAD_FORMAT;
    }
    uint32_t total = c->data_bits + (c->parity != UART_PARITY_NONE ? 1U : 0U);
    uint32_t v;
    switch(total){
    case 7U: v = USART_CR1_M1; break;
    case 8U: v = 0U;           break;
    case 9U: v = USART_CR1_M0; break;
    default: return ERR_UART_BAD_FORMAT;
    }
    if(c->parity != UART_PARITY_NONE){
        v |= USART_CR1_PCE;
        if(c->parity == UART_PARITY_ODD){
            v |= USART_CR1_PS;
        }
    }
    *cr1 = v;
    return ERR_OK;
}

err_t uart_init(USART_TypeDef *uart, const uart_config_t *config){
    if(uart == NULL || config == NULL){
        return ERR_NULL_PTR;
    }
    const uart_hw_t *hw = find_hw(uart);
    if(hw == NULL){
        return ERR_UART_BAD_INSTANCE;
    }

    /* Validate everything before touching the hardware */
    uint32_t cr1;
    err_t err = frame_bits(config, &cr1);
    if(err != ERR_OK) return err;
    uint32_t clk_hz;
    err = kernel_clock_hz(hw, config->clock, &clk_hz);
    if(err != ERR_OK) return err;
    uint32_t brr;
    err = baud_divider(hw, clk_hz, config->baud, &brr);
    if(err != ERR_OK) return err;

    err = rcc_periph_clock_enable(hw->periph);
    if(err != ERR_OK) return err;
    err = rcc_periph_reset(hw->periph);   /* start from reset values */
    if(err != ERR_OK) return err;

    /* Kernel clock selection: RCC_CCIPR is shared, so update it atomically */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    RCC->CCIPR = (RCC->CCIPR & ~(3UL << hw->ccipr_pos)) | ((uint32_t)config->clock << hw->ccipr_pos);
    __set_PRIMASK(primask);

    /* UE is 0 after reset, so every configuration register is writable */
    uart->PRESC = 0U;   /* kernel clock / 1 */
    uart->BRR = brr;
    uart->CR2 = (config->stop == UART_STOP_2) ? USART_CR2_STOP_1 : 0U;
    uart->CR3 = 0U;
    uart->CR1 = cr1 | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    /* Wait until the transmitter and receiver acknowledge being enabled */
    timeout_t t;
    timeout_start(&t, 10U);
    while((uart->ISR & (USART_ISR_TEACK | USART_ISR_REACK)) != (USART_ISR_TEACK | USART_ISR_REACK)){
        if(timeout_expired(&t)){
            return ERR_TIMEOUT;
        }
    }
    return ERR_OK;
}

err_t uart_deinit(USART_TypeDef *uart){
    if(uart == NULL){
        return ERR_NULL_PTR;
    }
    const uart_hw_t *hw = find_hw(uart);
    if(hw == NULL){
        return ERR_UART_BAD_INSTANCE;
    }
    uart->CR1 = 0U;
    err_t err = rcc_periph_reset(hw->periph);
    if(err != ERR_OK) return err;
    return rcc_periph_clock_disable(hw->periph);
}

/* ---------------------------------------------------------------------------
 *  Data path
 * ------------------------------------------------------------------------- */
err_t uart_write(USART_TypeDef *uart, const void *data, size_t len, uint32_t timeout_ms){
    err_t err = check_ready(uart);
    if(err != ERR_OK) return err;
    if(data == NULL && len != 0U){
        return ERR_NULL_PTR;
    }
    const uint8_t *p = data;
    timeout_t t;
    timeout_start(&t, timeout_ms);
    for(size_t i = 0; i < len; i++){
        while(!(uart->ISR & USART_ISR_TXE_TXFNF)){
            if(timeout_expired(&t)){
                return ERR_UART_TX_TIMEOUT;
            }
        }
        uart->TDR = p[i];
    }
    return ERR_OK;
}

err_t uart_flush(USART_TypeDef *uart, uint32_t timeout_ms){
    err_t err = check_ready(uart);
    if(err != ERR_OK) return err;
    timeout_t t;
    timeout_start(&t, timeout_ms);
    while(!(uart->ISR & USART_ISR_TC)){
        if(timeout_expired(&t)){
            return ERR_UART_TX_TIMEOUT;
        }
    }
    return ERR_OK;
}

/* Checks and clears line errors. Parity/framing/noise also drop the bad byte. */
static err_t take_line_error(USART_TypeDef *uart){
    uint32_t isr = uart->ISR;
    err_t err;
    if(isr & USART_ISR_PE){
        err = ERR_UART_PARITY;
    } else if(isr & USART_ISR_FE){
        err = ERR_UART_FRAMING;
    } else if(isr & USART_ISR_NE){
        err = ERR_UART_NOISE;
    } else if(isr & USART_ISR_ORE){
        uart->ICR = USART_ICR_ORECF;   /* RDR still holds a good byte: keep it */
        return ERR_UART_OVERRUN;
    } else {
        return ERR_OK;
    }
    if(isr & USART_ISR_RXNE_RXFNE){
        (void)uart->RDR;
    }
    uart->ICR = USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF;
    return err;
}

err_t uart_getc(USART_TypeDef *uart, uint8_t *byte){
    err_t err = check_ready(uart);
    if(err != ERR_OK) return err;
    if(byte == NULL){
        return ERR_NULL_PTR;
    }
    err = take_line_error(uart);
    if(err != ERR_OK) return err;
    if(!(uart->ISR & USART_ISR_RXNE_RXFNE)){
        return ERR_UART_RX_EMPTY;
    }
    *byte = (uint8_t)uart->RDR;
    return ERR_OK;
}

err_t uart_read(USART_TypeDef *uart, void *buf, size_t len, uint32_t timeout_ms, size_t *received){
    size_t n = 0U;
    err_t err = check_ready(uart);
    if(err == ERR_OK && buf == NULL && len != 0U){
        err = ERR_NULL_PTR;
    }
    if(err == ERR_OK){
        uint8_t *p = buf;
        timeout_t t;
        timeout_start(&t, timeout_ms);
        while(n < len){
            err = uart_getc(uart, &p[n]);
            if(err == ERR_OK){
                n++;
            } else if(err == ERR_UART_RX_EMPTY){
                err = ERR_OK;
                if(timeout_expired(&t)){
                    err = ERR_UART_RX_TIMEOUT;
                    break;
                }
            } else {
                break;   /* line error */
            }
        }
    }
    if(received != NULL){
        *received = n;
    }
    return err;
}
