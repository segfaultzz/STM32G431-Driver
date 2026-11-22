#include "debug.h"
#include "FreeRTOS.h"
#include "hardware/gpio.h"
#include "hardware/rcc.h"
#include "semphr.h"
#include "task.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define PRINTK_BUF_SIZE 160U   /* lives on the caller's stack */
#define TX_TIMEOUT_MS   100U

static bool s_ready;
static SemaphoreHandle_t s_lock;

err_t debug_init(uint32_t baud){
    err_t err = rcc_periph_clock_enable(RCC_PERIPH_GPIOA);
    if(err != ERR_OK) return err;

    const gpio_config_t tx = {
        .pins = DEBUG_TX_PIN, .mode = GPIO_MODE_AF, .pull = GPIO_PULL_NONE,
        .otype = GPIO_OTYPE_PUSH_PULL, .speed = GPIO_SPEED_LOW, .af = DEBUG_GPIO_AF,
    };
    const gpio_config_t rx = {
        .pins = DEBUG_RX_PIN, .mode = GPIO_MODE_AF, .pull = GPIO_PULL_UP,  /* idle high */
        .otype = GPIO_OTYPE_PUSH_PULL, .speed = GPIO_SPEED_LOW, .af = DEBUG_GPIO_AF,
    };
    err = gpio_init(DEBUG_GPIO_PORT, &tx);
    if(err != ERR_OK) return err;
    err = gpio_init(DEBUG_GPIO_PORT, &rx);
    if(err != ERR_OK) return err;

    const uart_config_t cfg = UART_CONFIG_8N1(baud);
    err = uart_init(DEBUG_UART, &cfg);
    if(err != ERR_OK) return err;

    if(s_lock == NULL){
        s_lock = xSemaphoreCreateMutex();   /* heap works before the scheduler */
        if(s_lock == NULL) return ERR_BUSY;
    }
    s_ready = true;
    return ERR_OK;
}

/* Only lock from task context with the scheduler running */
static bool lock(void){
    if(s_lock == NULL || __get_IPSR() != 0U ||
       xTaskGetSchedulerState() != taskSCHEDULER_RUNNING){
        return false;
    }
    (void)xSemaphoreTake(s_lock, portMAX_DELAY);
    return true;
}

static void unlock(bool locked){
    if(locked){
        (void)xSemaphoreGive(s_lock);
    }
}

/* Writes buf, turning "\n" into "\r\n" for serial terminals */
static void write_raw(const char *buf, size_t len){
    size_t start = 0U;
    for(size_t i = 0U; i < len; i++){
        if(buf[i] == '\n'){
            (void)uart_write(DEBUG_UART, &buf[start], i - start, TX_TIMEOUT_MS);
            (void)uart_write(DEBUG_UART, "\r\n", 2U, TX_TIMEOUT_MS);
            start = i + 1U;
        }
    }
    (void)uart_write(DEBUG_UART, &buf[start], len - start, TX_TIMEOUT_MS);
}

void debug_write(const char *buf, size_t len){
    if(!s_ready || buf == NULL){
        return;
    }
    bool locked = lock();
    write_raw(buf, len);
    unlock(locked);
}

/* Formats into buf; on truncation the end becomes "...\n" */
static size_t format(char *buf, size_t size, int n){
    if(n < 0){
        return 0U;
    }
    if((size_t)n >= size){
        buf[size - 5U] = '.';
        buf[size - 4U] = '.';
        buf[size - 3U] = '.';
        buf[size - 2U] = '\n';
        return size - 1U;
    }
    return (size_t)n;
}

/* Length actually stored by snprintf */
static size_t clamp_len(int n, size_t size){
    if(n < 0){
        return 0U;
    }
    return ((size_t)n >= size) ? size - 1U : (size_t)n;
}

int printk(const char *fmt, ...){
    if(!s_ready){
        return -1;
    }
    char buf[PRINTK_BUF_SIZE];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    debug_write(buf, format(buf, sizeof buf, n));
    return n;
}

void printk_err(const char *file, int line, err_t err, const char *fmt, ...){
    if(!s_ready){
        return;
    }
    /* Three pieces under one lock, so a long message can't push the
     * error name/description out of the buffer */
    char buf[PRINTK_BUF_SIZE];
    bool locked = lock();

    int n = snprintf(buf, sizeof buf, "[ERR] %s:%d ", file, line);
    write_raw(buf, clamp_len(n, sizeof buf));

    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    write_raw(buf, clamp_len(n, sizeof buf));

    n = snprintf(buf, sizeof buf, ": %s (0x%04X) %s\n",
                 err_name(err), (unsigned)err, err_desc(err));
    write_raw(buf, clamp_len(n, sizeof buf));

    unlock(locked);
}
