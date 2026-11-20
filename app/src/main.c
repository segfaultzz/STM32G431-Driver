#include "FreeRTOS.h"
#include "task.h"

int main(void)
{
    vTaskStartScheduler();

    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    for (;;) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    for (;;) {
    }
}
