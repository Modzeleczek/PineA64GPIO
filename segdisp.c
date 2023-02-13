#include "segdisp.h"

#include <unistd.h>
#include <stddef.h>

int segdisp_init(segdisp_context_t *ctx,
  gpio_context_t *gpio_ctx,
  uint8_t serial_clock_gpio,
  uint8_t serial_data_gpio,
  uint8_t latch_clock_gpio)
{
  if (ctx == NULL) return -1;

  if (gpio_ctx == NULL) return -2;
  ctx->gpio_ctx = gpio_ctx;

  ctx->serial_clock_gpio = serial_clock_gpio;
  ctx->serial_data_gpio = serial_data_gpio;
  ctx->latch_clock_gpio = latch_clock_gpio;

  // Clear serial data and latch clocks.
  gpio_set_value(ctx->gpio_ctx, ctx->serial_clock_gpio, 0);
  gpio_set_value(ctx->gpio_ctx, ctx->latch_clock_gpio, 0);

  // Let new serial and latch clock values physically propagate.
  BIT_BANGING_DELAY();
  return 0;
}

void segdisp_shift_out(segdisp_context_t *ctx, uint8_t value)
{
  uint8_t i;
  // The least significant bit of 'value[7:0]' goes first to the shift register.
  for (i = 0; i <= 7; ++i)
  {
    // shift_register_data <= value[i]
    gpio_set_value(ctx->gpio_ctx, ctx->serial_data_gpio, value >> i);
    BIT_BANGING_DELAY();
    // shift_register_serial_clock <= 1
    gpio_set_value(ctx->gpio_ctx, ctx->serial_clock_gpio, 1);
    BIT_BANGING_DELAY();
    // shift_register_serial_clock <= 0
    gpio_set_value(ctx->gpio_ctx, ctx->serial_clock_gpio, 0);
  }
  BIT_BANGING_DELAY();
}

void segdisp_latch_content(segdisp_context_t *ctx)
{
  gpio_set_value(ctx->gpio_ctx, ctx->latch_clock_gpio, 1);
  BIT_BANGING_DELAY();
  gpio_set_value(ctx->gpio_ctx, ctx->latch_clock_gpio, 0);
  BIT_BANGING_DELAY();
}

void segdisp_clear(segdisp_context_t *ctx)
{
  // Set the content of the shift register to 0000_0000_11111111.
  segdisp_shift_out(ctx, 0b11111111);
  segdisp_shift_out(ctx, 0b00000000);
  segdisp_latch_content(ctx);
}

void segdisp_display_digit(segdisp_context_t *ctx,
  uint8_t position, uint8_t digit_value, uint8_t enable_decimal_point)
{
  /* A 4-digit 8-segment display module contains two 8-bit 74HC595 shift
  registers connected in series. I will refer to them as first_register[7:0]
  and second_register[7:0].

  If first_register[7:4] has i-th bit set (1), i-th digit from the left will
  turn on its segments described in second_register[7:0].

  first_register[3:0] should always be cleared (0).

  If second_register[7:0] has i-th bit cleared (0), the digits described in
  first_register[7:4] will turn on their i-th segment.

  Segments in second_register[7:0] are described using the following indexes:
    7
  2   6
    1
  3   5
    4 0
  Cleared bit (0) enables the segment and set bit (1) disables it.

  An example of content of both 8-bit shift registers:
  first_register[7:4]>
  |    first_register[3:0] (always 0000)
  |    |    second_register[7:0]>
  |    |    |
  |    |    |
  |    |    |
  |    |    |
  |    |    76543210 (segment index)
  1010_0000_00001100
  Above bit sequence causes the module to display:
  | |3.| |3.|
  '|' separates two adjacent digits and space denotes an empty digit. */

  uint8_t enabled_segments[] =
  {
    //7654321
    0b0000001, // 0
    0b1001111, // 1
    0b0010010, // 2
    0b0000110, // 3
    0b1001100, // 4
    0b0100100, // 5
    0b0100000, // 6
    0b0001111, // 7
    0b0000000, // 8
    0b0000100, // 9
    0b0001000, // 10 = A
    0b1100000, // 11 = b
    0b1110010, // 12 = c
    0b1000010, // 13 = d
    0b0110000, // 14 = E
    0b0111000, // 15 = F
  };
  /* enable_decimal_point[7:0] is 8-bit variable.
  If its least significant bit is cleared, the separator will be disabled.
  If it is set, the separator will be enabled.
  Firstly, negate the variable's value because 0 causes the point segment
  to be turned on and 1 to be turned off. */
  enable_decimal_point = ~enable_decimal_point;
  // Clear all bits except 1 least significant.
  enable_decimal_point &= 0b00000001;

  /* From lookup table take the bit sequence describing the desired digit_value.
  Shift it 1-bit left and append decimal point bit. */
  uint8_t segments_with_point =
    (enabled_segments[digit_value] << 1) | enable_decimal_point;
  segdisp_shift_out(ctx, segments_with_point);

  // This implementation assumes that only one digit is turned on.
  uint8_t enabled_digits = ((0b1000 >> position) << 4) | 0b0000;
  segdisp_shift_out(ctx, enabled_digits);

  // Display the content of two shift registers.
  segdisp_latch_content(ctx);
}

void segdisp_display_number(segdisp_context_t *ctx, uint16_t number, uint8_t base)
{
  int8_t i = 0;
  for (i = 0; i <= 3; ++i)
  {
    segdisp_display_digit(ctx, i, number % base, 0);
    number /= base;
  }
}
