#include "board.h"

#include "stm32f446re.h"

#define LED_PIN 5UL

void board_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC_AHB1ENR;

    GPIOA_MODER &= ~(3UL << (LED_PIN * 2UL));
    GPIOA_MODER |=  (1UL << (LED_PIN * 2UL));
    GPIOA_OTYPER &= ~(1UL << LED_PIN);
    GPIOA_OSPEEDR &= ~(3UL << (LED_PIN * 2UL));
    GPIOA_PUPDR &= ~(3UL << (LED_PIN * 2UL));
}

void board_led_on(void)
{
    GPIOA_BSRR = 1UL << LED_PIN;
}

void board_led_off(void)
{
    GPIOA_BSRR = 1UL << (LED_PIN + 16UL);
}

void board_led_toggle(void)
{
    if ((GPIOA_ODR & (1UL << LED_PIN)) != 0U) {
        board_led_off();
    } else {
        board_led_on();
    }
}

void board_delay(volatile uint32_t ticks)
{
    while (ticks--) {
        __asm volatile ("nop");
    }
}
