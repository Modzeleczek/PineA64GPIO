#ifndef SEGDISP_H
#define SEGDISP_H

#include "gpio.h"

#define BIT_BANGING_DELAY 1

typedef struct
{
  gpio_context_t *gpio_ctx;
  uint8_t serial_clock_gpio,
    serial_data_gpio,
    latch_clock_gpio;
} segdisp_context_t;

int segdisp_init(segdisp_context_t *ctx,
  gpio_context_t *gpio_ctx,
  uint8_t serial_clock_gpio,
  uint8_t serial_data_gpio,
  uint8_t latch_clock_gpio);
void segdisp_shift_out(segdisp_context_t *ctx, uint8_t value);
void segdisp_latch_content(segdisp_context_t *ctx);
void segdisp_clear(segdisp_context_t *ctx);
void segdisp_display_digit(segdisp_context_t *ctx,
  uint8_t position, uint8_t digit_value, uint8_t enable_decimal_point);
void segdisp_display_number(segdisp_context_t *ctx, uint16_t number, uint8_t base);

#endif // SEGDISP_H
