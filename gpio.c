#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/* informacje o rejestrach dostępnych w procesorze Allwinner A64 według rozdziału
3.21. Port Controller(CPUx-PORT) zaczynającego się na stronie 376 pliku Allwinner_A64_User_Manual_V1.0.pdf */

#define MAP_SIZE (4*1024)
#define PAGE_SIZE (4*1024)
/* (str. 376) 0x01C20800 to adres bazowy modułu PIO w przestrzeni adresowej procesora;
(str. 377) pierwszy rejestr (PB_CFG0) znajduje się na offsecie 0x24 od adresu bazowego;
przekazujemy adres 0x01C20000 do mmap, bo musimy zachować wyrównanie (alignment) adresów,
tzn. adres przekazywany do mmap musi być podzielny (modulo) przez PAGE_SIZE */
#define GPIO_REGISTERS_BASE_ADDRESS 0x01C20000

typedef struct
{
  int dev_mem_fd;
  uint8_t *unaligned_mem;
  /* Allwinner A64 (według https://linux-sunxi.org/Allwinner_SoC_Family, seria A, rodzina sun50i)
  ma 32-bitowe rejestry PIO, stąd 4-bajtowy typ */
  uint8_t *map;
} gpio_context_t;

int gpio_init(gpio_context_t *ctx)
{
  if (ctx == NULL) return -1;

  // czemu musimy mapować /dev/mem w przestrzeni adresowej procesu zamiast po prostu pisać wskaźnikiem po pamięci: 
  // https://raspberrypi.stackexchange.com/a/636
  // urządzenie znakowe /dev/mem służy do czytania i pisania po przestrzeni adresowej procesora
  if ((ctx->dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
  {
    perror("init; open");
    gpio_cleanup(ctx);
    return -2;
  }

  if ((ctx->unaligned_mem = (uint8_t*)malloc(MAP_SIZE + (PAGE_SIZE - 1))) == NULL)
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
    MAP_SHARED | MAP_FIXED, ctx->dev_mem_fd, GPIO_REGISTERS_BASE_ADDRESS)) == MAP_FAILED)
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
      // stawiamy bity sygnalizujące, co zakończyło się błędem
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

// każdy port ma poniższe rejestry
typedef struct
{
  uint32_t CFG[4]; // Pn_CFG0-3
  uint32_t DAT; // Pn_DAT
  uint32_t DRV[2]; // Pn_DRV0-1
  uint32_t PUL[2]; // Pn_PUL0-1
} port_t;

/*
"gpio" zawiera indeks portu i 1-bitowej linii wejścia/wyjścia (io) w porcie
indeks portu    port    liczba io
0               B       10
1               C       17
2               D       25
3               E       18
4               F       7
5               G       14
6               H       12
np. 011 00110 - port 3 (E); io 6
*/
#define GPIO_PORT(gpio) ((gpio) >> 5)
#define PORT_OFFSET(map, port_id) ((port_t*)((uint8_t*)map + 0x800 + 0x24 + port_id * sizeof(port_t)))
#define GPIO_IO(gpio) ((gpio) & 0b00011111)

/*
"pull_up_down" może mieć poniższe wartości
00: Pull-up/down disable  01: Pull-up
10: Pull-down             11: Reserved
*/
void gpio_set_pull_up_down(gpio_context_t *ctx, uint8_t gpio, uint8_t pull_up_down)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111
  int reg_id = io_id >> 4; // (gpio & 0b000_11111) / 16
  int io_offset = (io_id & 0xF) << 1; // ((gpio & 0b000_11111) % 16) * 2

  uint32_t reg_val = port_offset->PUL[reg_id];
  // zerujemy 2 bity odpowiadające za pull-up/down linii wejścia/wyjścia o indeksie io_id
  reg_val &= ~(0b11 << io_offset);
  // zerujemy wszystkie bity poza 2 najmniej znaczącymi
  pull_up_down &= 0b00000011;
  // ustawiamy z pull_up_down 2 bity odpowiadające za pull-up/down linii wejścia/wyjścia o indeksie io_id
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
  // zerujemy wszystkie bity poza 2 odpowiadającymi za pull-up/down linii wejścia/wyjścia o indeksie io_id
  reg_val >>= io_offset;
  reg_val &= 0b11;
  return (uint8_t)reg_val;
}

/*
"function" może mieć poniższe wartości (na przykładzie PC13 ze strony 381; inne GPIO mogą mieć inne funkcje specjalne, tutaj NAND_DQ5, SDC2_D5)
000: Input     001: Output
010: NAND_DQ5  011: SDC2_D5
100: Reserved  101: Reserved
110: Reserved  111: IO Disable
*/
void gpio_set_function(gpio_context_t *ctx, uint8_t gpio, uint8_t function)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111
  int reg_id = io_id >> 3; // (gpio & 0b000_11111) / 8
  int io_offset = (io_id & 0x7) << 2; // ((gpio & 0b000_11111) % 8) * 4

  /* for debugging: printf("set_function; ctx->map %p; gpio %i, function %i; port_id %i; port_offset %p; io_id %i; reg_id %i; io_offset %i",\
    ctx->map, (uint32_t)gpio, (uint32_t)function, port_id, port_offset, io_id, reg_id, io_offset); */

  uint32_t reg_val = port_offset->CFG[reg_id]; // pobieramy wartość rejestru
  // printf("set_function; reg_val: %i", reg_val);
  // zerujemy 3 bity odpowiadające za funkcję linii wejścia/wyjścia o indeksie io_id
  reg_val &= ~(0b111 << io_offset);
  // zerujemy wszystkie bity poza 3 najmniej znaczącymi
  function &= 0b00000111;
  // ustawiamy z function 3 bity odpowiadające za funkcję linii wejścia/wyjścia o indeksie io_id
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

  uint32_t reg_val = port_offset->CFG[reg_id]; // pobieramy wartość rejestru
  // zerujemy wszystkie bity poza 3 odpowiadającymi za funkcję linii wejścia/wyjścia o indeksie io_id
  reg_val >>= io_offset;
  reg_val &= 0b111;
  return (uint8_t)reg_val;
}

/*
"value" może mieć wartość 0 lub 1
*/
void gpio_set_value(gpio_context_t *ctx, uint8_t gpio, uint8_t value)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111

  // jeżeli linia wejścia/wyjścia jest skonfigurowana jako output, to możemy zapisać na niej bit
  // zerujemy bit linii wejścia/wyjścia
  port_offset->DAT &= ~(1 << io_id);
  // zerujemy wszystkie bity poza 1 najmniej znaczącym
  value &= 0b00000001;
  // ustawiamy najmniej znaczący bit value jako jedyny bit linii wejścia/wyjścia
  port_offset->DAT |= value << io_id;
}

uint8_t gpio_get_value(gpio_context_t *ctx, uint8_t gpio)
{
  int port_id = GPIO_PORT(gpio); // gpio >> 5
  port_t *port_offset = PORT_OFFSET(ctx->map, port_id);
  int io_id = GPIO_IO(gpio); // gpio & 0b000_11111

  // jeżeli linia wejścia/wyjścia jest skonfigurowana jako input, to możemy odczytać z niej bit
  // jeżeli jako funkcjonalna, to odczytana wartość jest niezdefiniowana
  uint32_t reg_val = port_offset->DAT;
  // zerujemy wszystkie bity poza 1 najmniej znaczącym
  reg_val >>= io_id;
  reg_val &= 1;
  // rzutujemy, bo wszystkie bardziej znaczące bity niż pierwszy są wyzerowane
  return (uint8_t)reg_val;
}
