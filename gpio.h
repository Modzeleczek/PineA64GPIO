#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

typedef struct
{
  int dev_mem_fd;
  uint8_t *unaligned_mem;
  /* Allwinner A64 (według https://linux-sunxi.org/Allwinner_SoC_Family, seria A, rodzina sun50i)
  ma 32-bitowe rejestry PIO, stąd 4-bajtowy typ */
  uint8_t *map;
} gpio_context_t;

int gpio_init(gpio_context_t *ctx);
int gpio_cleanup(gpio_context_t *ctx);

/*
"gpio" zawiera indeks portu i wejścia/wyjścia (io) w porcie
indeks portu    port    liczba 1-bitowych linii wejścia/wyjścia
0               B       10
1               C       17
2               D       25
3               E       18
4               F       7
5               G       14
6               H       12
np. 011 00110 - port 3 (E); io 6
*/
/*
"pull_up_down" może mieć poniższe wartości
00: Pull-up/down disable  01: Pull-up
10: Pull-down             11: Reserved
*/
void gpio_set_pull_up_down(gpio_context_t *ctx, uint8_t gpio, uint8_t pull_up_down);
uint8_t gpio_get_pull_up_down(gpio_context_t *ctx, uint8_t gpio);

/*
"function" może mieć poniższe wartości (na przykładzie PC13 ze strony 381; inne GPIO mogą mieć inne funkcje specjalne, tutaj NAND_DQ5, SDC2_D5)
000: Input     001: Output
010: NAND_DQ5  011: SDC2_D5
100: Reserved  101: Reserved
110: Reserved  111: IO Disable
*/
void gpio_set_function(gpio_context_t *ctx, uint8_t gpio, uint8_t function);
uint8_t gpio_get_function(gpio_context_t *ctx, uint8_t gpio);

/*
"value" może mieć wartość 0 lub 1
*/
void gpio_set_value(gpio_context_t *ctx, uint8_t gpio, uint8_t value);
uint8_t gpio_get_value(gpio_context_t *ctx, uint8_t gpio);

#endif // GPIO_H
