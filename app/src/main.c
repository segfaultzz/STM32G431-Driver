#include "FreeRTOS.h"
#include "debug.h"
#include "hardware/gpio.h"
#include "hardware/rcc.h"
#include "task.h"

/* printk's vsnprintf needs more than the 128-word minimal stack */
#define BLINK_STACK_WORDS 384U

void vBlinkTask(void *pvParameters) {
  (void)pvParameters;

  ERR_CHECK(rcc_periph_clock_enable(RCC_PERIPH_GPIOB));
  ERR_CHECK(gpio_set_mode(GPIOB, GPIO_PIN_8, GPIO_MODE_OUTPUT));
  ERR_CHECK(gpio_set_output_type(GPIOB, GPIO_PIN_8, GPIO_OTYPE_PUSH_PULL));

  uint32_t count = 0;
  for (;;) {
    gpio_toggle_pin(GPIOB, GPIO_PIN_8);
    pr_info("blink %lu", (unsigned long)count++);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

int main(void) {
  /* Clock tree first: FreeRTOS reads SystemCoreClock when the scheduler
   * starts, and the UART baud divider depends on PCLK. On failure the chip
   * keeps running from HSI16, so the console still works. */
  err_t clk_err = rcc_clock_config(&rcc_config_hsi16_170mhz);

  if (debug_init(DEBUG_BAUD) != ERR_OK) {
    for (;;) { /* no console: nothing to print with */
    }
  }
  printk("\n--- STM32G431 driver demo ---\n");
  if (clk_err != ERR_OK) {
    pr_err(clk_err, "rcc_clock_config");
  }
  pr_info("SYSCLK %lu Hz, HCLK %lu Hz, PCLK1 %lu Hz, PCLK2 %lu Hz",
          (unsigned long)rcc_get_sysclk_hz(), (unsigned long)rcc_get_hclk_hz(),
          (unsigned long)rcc_get_pclk1_hz(), (unsigned long)rcc_get_pclk2_hz());

  if (xTaskCreate(vBlinkTask, "Blink", BLINK_STACK_WORDS, NULL,
                  tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
    pr_err(ERR_BUSY, "xTaskCreate(Blink): not enough heap");
  }
  vTaskStartScheduler();

  for (;;) {
  }
}

void vApplicationMallocFailedHook(void) {
  for (;;) {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name) {
  (void)task;
  /* Polled UART output still works here (the scheduler is stuck) */
  printk("\n[FATAL] stack overflow in task '%s'\n", name);
  for (;;) {
  }
}
