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

  // inicjujemy początkowe wartości zegarów
  gpio_set_value(ctx->gpio_ctx, ctx->serial_clock_gpio, 0);
#ifdef BIT_BANGING_DELAY
  usleep(BIT_BANGING_DELAY);
#endif
  gpio_set_value(ctx->gpio_ctx, ctx->latch_clock_gpio, 0);
#ifdef BIT_BANGING_DELAY
  usleep(BIT_BANGING_DELAY);
#endif

  return 0;
}

void segdisp_shift_out(segdisp_context_t *ctx, uint8_t value)
{
  // value[7:0]
  uint8_t i;
  // najmniej znaczący bit najpierw
  for (i = 0; i <= 7; ++i)
  {
    gpio_set_value(ctx->gpio_ctx, ctx->serial_data_gpio, value >> i); // data = value[7-i]
  #ifdef BIT_BANGING_DELAY
    usleep(BIT_BANGING_DELAY);
  #endif
    gpio_set_value(ctx->gpio_ctx, ctx->serial_clock_gpio, 1); // clock = 1
  #ifdef BIT_BANGING_DELAY
    usleep(BIT_BANGING_DELAY);
  #endif
    gpio_set_value(ctx->gpio_ctx, ctx->serial_clock_gpio, 0); // clock = 0
  #ifdef BIT_BANGING_DELAY
    usleep(BIT_BANGING_DELAY);
  #endif
  }
}

void segdisp_latch_content(segdisp_context_t *ctx)
{
  gpio_set_value(ctx->gpio_ctx, ctx->latch_clock_gpio, 1);
#ifdef BIT_BANGING_DELAY
  usleep(BIT_BANGING_DELAY);
#endif
  gpio_set_value(ctx->gpio_ctx, ctx->latch_clock_gpio, 0);
#ifdef BIT_BANGING_DELAY
  usleep(BIT_BANGING_DELAY);
#endif
}

void segdisp_clear(segdisp_context_t *ctx)
{
  // wpisujemy 0000_0000_11111111
  segdisp_shift_out(ctx, 0b11111111);
  segdisp_shift_out(ctx, 0b00000000);
  segdisp_latch_content(ctx);
}

void segdisp_display_digit(segdisp_context_t *ctx,
  uint8_t position, uint8_t digit_value, uint8_t enable_decimal_point)
{
  /*
  <lewy rejestr[7:4]> zawiera 1 na i-tej pozycji od prawej,
  jeżeli i-ta cyfra od lewej ma mieć włączone segmenty opisane w <prawy rejestr[7:0]>

  <prawy rejestr[7:0]> zawiera 0 na i-tej pozycji od prawej,
  jeżeli i-ty segment ma być włączony
  
    7 
  2   6
    1
  3   5
    4 0

  <lewy rejestr[7:4]>
  |    <lewy rejestr[3:0]> (zawsze 0000)
  |    |    <prawy rejestr[7:0]>
  |    |    |
  |    |    |
  |    |    |
  |    |    |
  |    |    76543210
  1010_0000_00001101;
  */
  
  /* bit równy 0 - cyfra aktywna
    7
  2   6
    1
  3   5
    4 0
  */

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
  // 0 == false; 1 == true, ale 0 na pozycji 0 włącza segment kropki
  enable_decimal_point = ~enable_decimal_point;
  // zerujemy wszystkie bity poza najmniej znaczącym
  enable_decimal_point &= 0b00000001;

  uint8_t segments_with_dot =
    (enabled_segments[digit_value] << 1) | enable_decimal_point;
  segdisp_shift_out(ctx, segments_with_dot);

  uint8_t enabled_digits = ((0b1000 >> position) << 4) | 0b0000;
  segdisp_shift_out(ctx, enabled_digits);

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
