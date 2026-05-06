#include "board.h"

int main(void)
{
    board_init();

    while (1) {
        board_led_on();
        board_delay(500000UL);
        board_led_off();
        board_delay(500000UL);
    }
}
