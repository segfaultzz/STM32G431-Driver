#include "hardware/gpio.h"
#include <stddef.h>

/*
 * Writes 'value' into the 'width'-bit field of every pin in 'pins'.
 * All pins are updated with ONE register write, so a pin never passes through
 * an in-between state (clearing MODER first would briefly make it an input).
 */
static void write_fields(volatile uint32_t *reg, uint16_t pins, uint32_t width, uint32_t value){
    uint32_t field = (1U << width) - 1U;
    uint32_t clear = 0U;
    uint32_t set = 0U;

    for(uint32_t pin = 0U; pin < 16U; pin++){
        if(pins & (1U << pin)){
            clear |= field << (pin * width);
            set |= (value & field) << (pin * width);
        }
    }
    *reg = (*reg & ~clear) | set;
}

err_t gpio_set_mode(GPIO_TypeDef *port, uint16_t pins, gpio_mode_t mode){
    if(port == NULL || pins == 0U || mode > GPIO_MODE_ANALOG){
        return ERR_INVALID_PARAM;
    }
    write_fields(&port->MODER, pins, 2U, (uint32_t)mode);
    return ERR_OK;
}

err_t gpio_set_pull(GPIO_TypeDef *port, uint16_t pins, gpio_pull_t pull){
    if(port == NULL || pins == 0U || pull > GPIO_PULL_DOWN){
        return ERR_INVALID_PARAM;
    }
    write_fields(&port->PUPDR, pins, 2U, (uint32_t)pull);
    return ERR_OK;
}

err_t gpio_set_output_type(GPIO_TypeDef *port, uint16_t pins, gpio_otype_t otype){
    if(port == NULL || pins == 0U || otype > GPIO_OTYPE_OPEN_DRAIN){
        return ERR_INVALID_PARAM;
    }
    write_fields(&port->OTYPER, pins, 1U, (uint32_t)otype);
    return ERR_OK;
}

err_t gpio_set_output_speed(GPIO_TypeDef *port, uint16_t pins, gpio_speed_t speed){
    if(port == NULL || pins == 0U || speed > GPIO_SPEED_VERY_HIGH){
        return ERR_INVALID_PARAM;
    }
    write_fields(&port->OSPEEDR, pins, 2U, (uint32_t)speed);
    return ERR_OK;
}

err_t gpio_set_af(GPIO_TypeDef *port, uint16_t pins, uint8_t af){
    if(port == NULL || pins == 0U || af > 15U){
        return ERR_INVALID_PARAM;
    }
    /* AFR[0] holds pins 0..7, AFR[1] pins 8..15, 4 bits each */
    uint16_t low = pins & 0x00FFU;
    uint16_t high = (uint16_t)(pins >> 8);
    if(low){
        write_fields(&port->AFR[0], low, 4U, af);
    }
    if(high){
        write_fields(&port->AFR[1], high, 4U, af);
    }
    return ERR_OK;
}

err_t gpio_init(GPIO_TypeDef *port, const gpio_config_t *config){
    if(port == NULL || config == NULL || config->pins == 0U){
        return ERR_INVALID_PARAM;
    }
    err_t err;

    /* Everything else first, mode last, so the pin doesn't glitch while being set up */
    err = gpio_set_output_type(port, config->pins, config->otype);
    if(err != ERR_OK) return err;
    err = gpio_set_output_speed(port, config->pins, config->speed);
    if(err != ERR_OK) return err;
    err = gpio_set_pull(port, config->pins, config->pull);
    if(err != ERR_OK) return err;
    if(config->mode == GPIO_MODE_AF){
        err = gpio_set_af(port, config->pins, config->af);
        if(err != ERR_OK) return err;
    }
    return gpio_set_mode(port, config->pins, config->mode);
}

err_t gpio_reset_pin(GPIO_TypeDef *port, uint16_t pins){
    if(port == NULL || pins == 0U){
        return ERR_INVALID_PARAM;
    }
    /* Mode first this time: the pin stops driving (analog) before the rest changes */
    write_fields(&port->MODER, pins, 2U, (uint32_t)GPIO_MODE_ANALOG);
    write_fields(&port->PUPDR, pins, 2U, (uint32_t)GPIO_PULL_NONE);
    write_fields(&port->OTYPER, pins, 1U, (uint32_t)GPIO_OTYPE_PUSH_PULL);
    write_fields(&port->OSPEEDR, pins, 2U, (uint32_t)GPIO_SPEED_LOW);
    uint16_t low = pins & 0x00FFU;
    uint16_t high = (uint16_t)(pins >> 8);
    if(low){
        write_fields(&port->AFR[0], low, 4U, 0U);
    }
    if(high){
        write_fields(&port->AFR[1], high, 4U, 0U);
    }
    port->BSRR = (uint32_t)pins << 16;     /* output latch back to 0 */
    return ERR_OK;
}

err_t gpio_write_pin(GPIO_TypeDef *port, uint16_t pins, uint16_t value){
    if(port == NULL || pins == 0U){
        return ERR_INVALID_PARAM;
    }
    /* BSRR: low half sets, high half resets. Atomic, no read-modify-write */
    if(value){
        port->BSRR = pins;
    } else {
        port->BSRR = (uint32_t)pins << 16;
    }
    return ERR_OK;
}

uint32_t gpio_read_pin(GPIO_TypeDef *port, uint16_t pin){
    if(port == NULL || pin == 0U){
        return 0U;
    }
    return (port->IDR & pin) ? 1U : 0U;
}

err_t gpio_toggle_pin(GPIO_TypeDef *port, uint16_t pin){
    if(port == NULL || pin == 0U){
        return ERR_INVALID_PARAM;
    }
    /* High pins get reset, low pins get set, in one BSRR write */
    uint32_t odr = port->ODR;
    port->BSRR = ((odr & pin) << 16) | (~odr & pin);
    return ERR_OK;
}

err_t gpio_lock_pin(GPIO_TypeDef *port, uint16_t pin){
    if(port == NULL || pin == 0U){
        return ERR_INVALID_PARAM;
    }
    /* RM0440 lock key sequence: write 1+pins, 0+pins, 1+pins, then read twice.
     * Must not be interrupted, so interrupts are masked for these few writes.
     * The pin configuration stays frozen until the next reset. */
    uint32_t key = GPIO_LCKR_LCKK | pin;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    port->LCKR = key;
    port->LCKR = pin;
    port->LCKR = key;
    (void)port->LCKR;
    uint32_t locked = port->LCKR & GPIO_LCKR_LCKK;
    __set_PRIMASK(primask);

    return locked ? ERR_OK : ERR_TIMEOUT;
}

err_t gpio_write_port(GPIO_TypeDef *port, uint16_t value){
    if(port == NULL){
        return ERR_INVALID_PARAM;
    }
    /* Set the 1 bits and reset the 0 bits in one BSRR write */
    port->BSRR = ((uint32_t)(uint16_t)~value << 16) | value;
    return ERR_OK;
}

err_t gpio_read_port(GPIO_TypeDef *port, uint16_t *value){
    if(port == NULL || value == NULL){
        return ERR_INVALID_PARAM;
    }
    *value = (uint16_t)port->IDR;
    return ERR_OK;
}
