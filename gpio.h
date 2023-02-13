#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

/* Struct for simulating object oriented programming in C by
passing its instance to library functions. */
typedef struct
{
  int dev_mem_fd;
  uint8_t *unaligned_mem;
  uint8_t *map;
} gpio_context_t;

/*
Initializes gpio_context_t struct instance pointed by 'ctx'.

Negative return value denotes an error and 0 means success.
*/
int gpio_init(gpio_context_t *ctx);

/*
Releases resources used by gpio_context_t struct instance pointed by 'ctx'.

Positive return value denotes an error and 0 means success.
*/
int gpio_cleanup(gpio_context_t *ctx);

/*
Sets pull-up/down resistor mode of 'gpio'.

'gpio' contains GPIO port index in 3 more significant bits
and 1-bit input/output line index in 5 less significant bits.
port index      port    number of 1-bit input/output lines (manual, p. 376)
0               B       10
1               C       17
2               D       25
3               E       18
4               F       7
5               G       14
6               H       12
e.g. 011 00110 - port 3 (E), io 6

'pull_up_down' can have the following values (manual, p. 379):
00: Pull-up/down disable  01: Pull-up
10: Pull-down             11: Reserved
*/
void gpio_set_pull_up_down(gpio_context_t *ctx, uint8_t gpio, uint8_t pull_up_down);

/*
Gets pull-up/down resistor mode of 'gpio'.

The read value is stored in 2 least significant bits of returned uint8_t.
*/
uint8_t gpio_get_pull_up_down(gpio_context_t *ctx, uint8_t gpio);

/*
Sets function mode of 'gpio'.

'function' can have varying value depending on the selected GPIO port
and input/output line.

An example for gpio == 001 01101 (PC13) (manual, p. 381) which has NAND_DQ5 and
SDC2_D5 special functions:
000: Input     001: Output
010: NAND_DQ5  011: SDC2_D5
100: Reserved  101: Reserved
110: Reserved  111: IO Disable

Other GPIO ports and input/output lines can have different special functions
but the basic values 000 (Input), 001 (Output), 111 (IO Disable) should remain
the same.
*/
void gpio_set_function(gpio_context_t *ctx, uint8_t gpio, uint8_t function);

/*
Gets function mode of 'gpio'.

The read value is stored in 3 least significant bits of returned uint8_t.
*/
uint8_t gpio_get_function(gpio_context_t *ctx, uint8_t gpio);

/*
Gets value of 'gpio'.

'value' can have value 0 or 1.
*/
void gpio_set_value(gpio_context_t *ctx, uint8_t gpio, uint8_t value);

/*
Sets value of 'gpio'.

The read value is stored in 1 least significant bit of returned uint8_t.
*/
uint8_t gpio_get_value(gpio_context_t *ctx, uint8_t gpio);

#endif // GPIO_H
