#ifndef SEGDISP_H
#define SEGDISP_H

#include "gpio.h"

/* Set it to slow down shift register operations in case they are performed on
a slow device. 'usleep' function causes the process to sleep for given number
of microseconds. */
#define BIT_BANGING_DELAY() (usleep(1))
// Uncomment the lines below to disable the delay.
#undef BIT_BANGING_DELAY
#define BIT_BANGING_DELAY()

typedef struct
{
  gpio_context_t *gpio_ctx;
  uint8_t serial_clock_gpio,
    serial_data_gpio,
    latch_clock_gpio;
} segdisp_context_t;

/*
Initializes segdisp_context_t struct instance pointed by 'ctx'.

Negative return value denotes an error and 0 means success.
*/
int segdisp_init(segdisp_context_t *ctx,
  gpio_context_t *gpio_ctx,
  uint8_t serial_clock_gpio,
  uint8_t serial_data_gpio,
  uint8_t latch_clock_gpio);

/*
Inserts 'value' into shift register.

'value' contains the sentence of bits to be inserted into shift register.
The least significant bit of 'value' goes first.
*/
void segdisp_shift_out(segdisp_context_t *ctx, uint8_t value);

/*
Lets the content of the shift register into its parallel outputs.
*/
void segdisp_latch_content(segdisp_context_t *ctx);

/*
Clears the display by setting the content of the shift register
to 0000_0000_11111111.
*/
void segdisp_clear(segdisp_context_t *ctx);

/*
Displays one digit on the specified position with or without displaying
the decimal point.

'position' can have value in range {0..3}.
0 is the rightmost digit and 3 is the leftmost digit.

'digit_value' can have value in range {0..16}.

'enable_decimal_point' can have value 0 or 1.
0 is decimal point disabled and 1 is enabled.
*/
void segdisp_display_digit(segdisp_context_t *ctx,
  uint8_t position, uint8_t digit_value, uint8_t enable_decimal_point);

/*
Displays 'number' expressed in 'base' numeral system.

'number' is the number to be displayed.

'base' can have value in range {2..16}.
Values greater than 16 are not supported because segdisp_display_digit
function does not have definitions for digits greater than F (hexadecimal 15).
*/
void segdisp_display_number(segdisp_context_t *ctx, uint16_t number, uint8_t base);

#endif // SEGDISP_H
