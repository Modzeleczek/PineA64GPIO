#include "../gpio.h"

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

  const uint8_t button = 0b00101010; // PC10; pin 38
  gpio_set_function(&gpio_ctx, button, 0b000); // input
  gpio_set_pull_up_down(&gpio_ctx, button, 0b01); // pull-up

  const uint8_t led = 0b00101011; // PC11; pin 40
  gpio_set_function(&gpio_ctx, led, 0b001); // output

  gpio_set_value(&gpio_ctx, button, 0);

  while (1)
  {
    uint8_t button_value = gpio_get_value(&gpio_ctx, button);
    gpio_set_value(&gpio_ctx, led, button_value);
    usleep(1000);
  }

  if ((status = gpio_cleanup(&gpio_ctx)) != 0)
  {
    printf("cleanup error %i\n", status);
    return status;
  }
  return 0;
}
