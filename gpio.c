#include "gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/* Information about GPIO registers available in Allwinner A64 microprocessor
(hereinafter referred to as A64) was taken from chapter
3.21. Port Controller(CPUx-PORT) beginning on page 376 of
(https://linux-sunxi.org/images/b/b4/Allwinner_A64_User_Manual_V1.1.pdf
accessed 11.02.2023) */

#define MAP_SIZE (4*1024)
#define PAGE_SIZE (4*1024)
/* (p. 376) 0x01C20800 is base address of the GPIO (in the manual called PIO)
module in A64 address space.
(p. 377) The first register (PB_CFG0) is offset 0x24 from base address.
We pass address 0x01C20000 to mmap instead of 0x01C20800 in order to retain
address alignment, i.e. address passed to mmap must be divisible by PAGE_SIZE. */
#define GPIO_REGISTERS_BASE_ADDRESS 0x01C20000

int gpio_init(gpio_context_t *ctx)
{
  if (ctx == NULL) return -1;

  /* Why we have to map /dev/mem in process address space instead of directly
  accessing memory using pointers: https://raspberrypi.stackexchange.com/a/636
  Characted device /dev/mem is used for reading and writing microprocessor's
  address space. */
  if ((ctx->dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
  {
    perror("init; open");
    gpio_cleanup(ctx);
    return -2;
  }

  if ((ctx->unaligned_mem = (uint8_t*)malloc(MAP_SIZE + (PAGE_SIZE - 1))) ==
    NULL)
  {
    gpio_cleanup(ctx);
    return -3;
  }

  uint8_t *aligned_mem = ctx->unaligned_mem;
  size_t remainder = (size_t)aligned_mem % PAGE_SIZE;
  if (remainder != 0)
    aligned_mem += PAGE_SIZE - remainder;
  /* for debugging: printf("unaligned_mem: %p\naligned_mem: %p\nremainder: %lu",
    ctx->unaligned_mem, aligned_mem, remainder); */

  if ((ctx->map = (uint8_t*)mmap(aligned_mem, MAP_SIZE, PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_FIXED, ctx->dev_mem_fd, GPIO_REGISTERS_BASE_ADDRESS)) ==
    MAP_FAILED)
  {
    perror("init; mmap");
    gpio_cleanup(ctx);
    return -4;
  }

  return 0;
}

int gpio_cleanup(gpio_context_t *ctx)
{
  int ret = 0;
  if (ctx->unaligned_mem != NULL)
    free(ctx->unaligned_mem);

  if (ctx->map != MAP_FAILED) // -1
    if (munmap(ctx->map, MAP_SIZE) == -1)
    {
      // Set bits denoting what finished with an error.
      ret |= (1 << 0);
      perror("cleanup; munmap");
    }

  if (ctx->dev_mem_fd != -1)
    if (close(ctx->dev_mem_fd) == -1)
    {
      ret |= (1 << 1);
      perror("cleanup; close");
    }

  return ret;
}

// Every A64 GPIO port has registers described by the following struct.
typedef struct
{
  uint32_t CFG[4]; // Pn_CFG[3:0]
  uint32_t DAT; // Pn_DAT
  uint32_t DRV[2]; // Pn_DRV[1:0]
  uint32_t PUL[2]; // Pn_PUL[1:0]
} port_t;

#define GPIO_PORT(gpio) ((gpio) >> 5)
/* Add 0x800 to mmap-aligned address 0x01C20000 in order to obtain GPIO
base address 0x01C20800. Then add 0x24 (manual, p. 377) to get to the beginning
of the first GPIO port in A64 address space which is PB. */
#define PORT_OFFSET(map, port_id) ((port_t*)((uint8_t*)map + 0x800 + 0x24 +\
  port_id * sizeof(port_t)))
#define GPIO_IO(gpio) ((gpio) & 0b00011111)

void gpio_set_pull_up_down(gpio_context_t *ctx, uint8_t gpio, uint8_t pull_up_down)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111
  int reg_id = io_id >> 4; // (gpio & 0b000_11111) / 16
  int io_offset = (io_id & 0xF) << 1; // ((gpio & 0b000_11111) % 16) * 2

  uint32_t reg_val = port_offset->PUL[reg_id];
  /* Clear 2 bits responsible for pull-up/down resistor of input/output line
  with index io_id. */
  reg_val &= ~(0b11 << io_offset);
  // Clear all bits except 2 least significant.
  pull_up_down &= 0b00000011;
  /* From 'pull_up_down' set 2 bits responsible for pull-up/down resistor
  of input/output line with index io_id. */
  reg_val |= pull_up_down << io_offset;
  port_offset->PUL[reg_id] = reg_val;
}

uint8_t gpio_get_pull_up_down(gpio_context_t *ctx, uint8_t gpio)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111
  int reg_id = io_id >> 4; // (gpio & 0b000_11111) / 16
  int io_offset = (io_id & 0xF) << 1; // ((gpio & 0b000_11111) % 16) * 2

  uint32_t reg_val = port_offset->PUL[reg_id];
  /* Clear all bits except 2 responsible for pull-up/down resistor
  of input/output line with index io_id. */
  reg_val >>= io_offset;
  reg_val &= 0b11;
  return (uint8_t)reg_val;
}

void gpio_set_function(gpio_context_t *ctx, uint8_t gpio, uint8_t function)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111
  int reg_id = io_id >> 3; // (gpio & 0b000_11111) / 8
  int io_offset = (io_id & 0x7) << 2; // ((gpio & 0b000_11111) % 8) * 4

  /* for debugging: printf("set_function; ctx->map %p; gpio %i, function %i; \
  port_id %i; port_offset %p; io_id %i; reg_id %i; io_offset %i",
    ctx->map, (uint32_t)gpio, (uint32_t)function, port_id, port_offset, io_id,
    reg_id, io_offset); */

  uint32_t reg_val = port_offset->CFG[reg_id]; // Get register value.
  /* Clear 3 bits responsible for function of input/output line
  with index io_id. */
  reg_val &= ~(0b111 << io_offset);
  // Clear all bits except 3 least significant.
  function &= 0b00000111;
  /* From 'function' set 3 bits responsible for function of input/output line
  with index io_id. */
  reg_val |= function << io_offset;
  port_offset->CFG[reg_id] = reg_val;
}

uint8_t gpio_get_function(gpio_context_t *ctx, uint8_t gpio)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111
  int reg_id = io_id >> 3; // (gpio & 0b000_11111) / 8
  int io_offset = (io_id & 0x7) << 2; // ((gpio & 0b000_11111) % 8) * 4

  uint32_t reg_val = port_offset->CFG[reg_id]; // Get register value.
  /* Clear all bits except 3 responsible for function of input/output line
  with index io_id. */
  reg_val >>= io_offset;
  reg_val &= 0b111;
  return (uint8_t)reg_val;
}

void gpio_set_value(gpio_context_t *ctx, uint8_t gpio, uint8_t value)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111

  // If input/output line is configured as output, we can write 1 bit to it.
  // Clear 1 bit of input/output line.
  port_offset->DAT &= ~(1 << io_id);
  // Clear all bits except 1 least significant.
  value &= 0b00000001;
  // Set 1 least significant bit of 'value' as the only bit of input/output line.
  port_offset->DAT |= value << io_id;
}

uint8_t gpio_get_value(gpio_context_t *ctx, uint8_t gpio)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111

  /* If input/output line is configured as input, we can read 1 bit from it.
  If it is configured as special function, value read from it is undefined. */
  uint32_t reg_val = port_offset->DAT;
  // Clear all bits except 1 least significant.
  reg_val >>= io_id;
  reg_val &= 1;
  /* All bits except 1 least significant are cleared so we can safely cast
  uint32_t to uint8_t. */
  return (uint8_t)reg_val;
}
