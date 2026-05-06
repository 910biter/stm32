#ifndef STM32F446RE_H
#define STM32F446RE_H

#include <stdint.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_BASE        0x40023800UL
#define GPIOA_BASE      0x40020000UL

#define RCC_AHB1ENR     REG32(RCC_BASE + 0x30UL)
#define GPIOA_MODER     REG32(GPIOA_BASE + 0x00UL)
#define GPIOA_OTYPER    REG32(GPIOA_BASE + 0x04UL)
#define GPIOA_OSPEEDR   REG32(GPIOA_BASE + 0x08UL)
#define GPIOA_PUPDR     REG32(GPIOA_BASE + 0x0CUL)
#define GPIOA_ODR       REG32(GPIOA_BASE + 0x14UL)
#define GPIOA_BSRR      REG32(GPIOA_BASE + 0x18UL)

#define RCC_AHB1ENR_GPIOAEN (1UL << 0)

#endif
