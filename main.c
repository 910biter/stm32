#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_BASE        0x40023800UL
#define GPIOA_BASE      0x40020000UL

#define RCC_AHB1ENR     REG32(RCC_BASE + 0x30UL)
#define GPIOA_MODER     REG32(GPIOA_BASE + 0x00UL)
#define GPIOA_OTYPER    REG32(GPIOA_BASE + 0x04UL)
#define GPIOA_OSPEEDR   REG32(GPIOA_BASE + 0x08UL)
#define GPIOA_PUPDR     REG32(GPIOA_BASE + 0x0CUL)
#define GPIOA_BSRR      REG32(GPIOA_BASE + 0x18UL)

#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define LED_PIN              5UL

static void delay(volatile uint32_t ticks)
{
    while (ticks--) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC_AHB1ENR;

    GPIOA_MODER &= ~(3UL << (LED_PIN * 2UL));
    GPIOA_MODER |=  (1UL << (LED_PIN * 2UL));
    GPIOA_OTYPER &= ~(1UL << LED_PIN);
    GPIOA_OSPEEDR &= ~(3UL << (LED_PIN * 2UL));
    GPIOA_PUPDR &= ~(3UL << (LED_PIN * 2UL));

    while (1) {
        GPIOA_BSRR = 1UL << LED_PIN;
        delay(500000UL);
        GPIOA_BSRR = 1UL << (LED_PIN + 16UL);
        delay(500000UL);
    }
}
