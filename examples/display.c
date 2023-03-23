#include "gpio.h"
#include "segdisp.h"

#include <stdio.h>
#include <unistd.h>

int main()
{
  gpio_context_t gpio_ctx;
  int status = 0;
  if ((status = gpio_init(&gpio_ctx)) != 0)
  {
    printf("init error %i\n", status);
    return status;
  }

  const uint8_t sclk = 0b00100110; // PC6; pin 36
  gpio_set_function(&gpio_ctx, sclk, 0b001); // output

  const uint8_t rclk = 0b00101010; // PC10; pin 38
  gpio_set_function(&gpio_ctx, rclk, 0b001); // output

  const uint8_t dio = 0b00101011; // PC11; pin 40
  gpio_set_function(&gpio_ctx, dio, 0b001); // output

  segdisp_context_t segdisp_ctx;
  segdisp_init(&segdisp_ctx, &gpio_ctx, sclk, dio, rclk);

  segdisp_clear(&segdisp_ctx);

  int counter = 0;
  while (1)
  {
    if (counter > 9999)
      counter = 0;

    size_t i;
    for (i = 0; i < 60; ++i)
    {
      segdisp_display_number(&segdisp_ctx, counter, 10);
      usleep(200);
    }

    ++counter;
  }

  if ((status = gpio_cleanup(&gpio_ctx)) != 0)
  {
    printf("cleanup error %i\n", status);
    return status;
  }
  return 0;
}
