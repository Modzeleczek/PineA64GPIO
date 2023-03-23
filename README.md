# PineA64GPIO
## Description
This repository contains GPIO library for Pine A64/A64+ single board computers based on Allwinner A64 CPU. Currently, it only supports programmed input-output without interrupts. It was tested on Pine A64+ running Armbian 22.11.1 Jammy with Linux 5.15.80-sunxi64 kernel.

### Hardware
The image below shows the position of connectors on Pine A64/A64+ board.

<img src="https://i.imgur.com/iIXpLUc.png" width="500"/>

Source (accessed 13.02.2023): [wiki link](https://wiki.pine64.org/wiki/PINE_A64#Information,_Schematics_and_Certifications), PINE A64 Connector Layout @courtesy of norm24 ([PNG link](https://wiki.pine64.org/images/7/7d/Pine64_Board_Connector.png))

Allwinner A64 user manual ([PDF link](https://linux-sunxi.org/images/b/b4/Allwinner_A64_User_Manual_V1.1.pdf), accessed 13.02.2023) on page 376 states that the CPU has 7 GPIO ports, each containing a number of configurable 1-bit input/output lines.
- Port B (PB): 10 i/o
- Port C (PC): 17 i/o
- Port D (PD): 25 i/o
- Port E (PE): 18 i/o
- Port F (PF): 7 i/o
- Port G (PG): 14 i/o
- Port H (PH): 12 i/o

According to file 'PINE A64 Pi-2/Eular/Ext Bus/Wifi Bus Connector Pin Assignment (Updated 15/Feb/2016)' ([PDF link](https://files.pine64.org/doc/Pine%20A64%20Schematic/Pine%20A64%20Pin%20Assignment%20160215.pdf), accessed 13.02.2023) from Pine A64 wiki, the listed above i/o lines are available through four goldpin connectors located on the board. The following tables show goldpin indexes and their corresponding i/o lines, e.g. i/o line PC6 can be accessed through 36-th goldpin of Pi-2 Connector.

<img src="https://i.imgur.com/gSe28II.png" width="500"/>


<img src="https://i.imgur.com/1Xq2Rlx.png" width="500"/>


<img src="https://i.imgur.com/IuuGhSI.png" width="500"/>


<img src="https://i.imgur.com/tFX6UHp.png" width="500"/>

## Building
GPIO library only uses Linux system calls and C standard library functions without any external dependencies. To build it, firstly clone this repository to a Pine A64/A64+ board running Linux. Make sure you have `gcc` (GNU Compiler Collection) and `ar` (Archiver) available. Then you have two options:
1. If `make` (GNU Make) is available, build the library using the following command:
    ```
    make libgpio
    ```

or

2. Permit execution of build.sh Bash script and run it:
    ```
    chmod 755 build.sh
    ./build.sh
    ```

Either way, compiled static library archive `libgpio.a` is placed in `./build` directory.

To delete `./build`, use:
```
make clean
```

## Usage
### Programming
1. In your code, create `gpio_context_t` struct instance and initialize it with `int gpio_init(gpio_context_t *ctx)`.

2. Using the tables in [PDF link](https://files.pine64.org/doc/Pine%20A64%20Schematic/Pine%20A64%20Pin%20Assignment%20160215.pdf), select a GPIO pin and check which i/o line symbol corresponds to the pin.

    E.g. pin 38 of Pi-2 Connector and i/o line PC10.

3. Set the i/o line's function with `void gpio_set_function(gpio_context_t *ctx, uint8_t gpio, uint8_t function)`.

    `gpio` parameter contains GPIO port index in 3 more significant bits  
    and i/o line index in 5 less significant bits.

    | port index | port symbol | number of i/o lines |
    |------------|-------------|---------------------|
    | 0          | PB          | 10                  |
    | 1          | PC          | 17                  |
    | 2          | PD          | 25                  |
    | 3          | PE          | 18                  |
    | 4          | PF          | 7                   |
    | 5          | PG          | 14                  |
    | 6          | PH          | 12                  |

    E.g. for i/o line PC10, `gpio` = 001 01010

    According to Allwinner A64 user manual, i/o line's basic function options are represented by the following bit sequences:
    - 000: Input
    - 001: Output
    - 111: IO Disable

    Various GPIO i/o lines can have different special functions (e.g. for hardware SPI protocol, UART, etc.) but three basic ones listed above should remain the same for all i/o lines.

    Pass the selected option in 3 least significant bits of `function` parameter.

    Example `gpio_set_function` call for setting PC10 i/o line as output.  
    `gpio_set_function(&ctx, 0b00101010, 0b001)`

4. (Optional, do it only with input i/o line function) Set the i/o line's internal pull-up/down resistor with `void gpio_set_pull_up_down(gpio_context_t *ctx, uint8_t gpio, uint8_t pull_up_down)`.

    For `gpio` parameter look up to paragraph 3.

    According to Allwinner A64 user manual, i/o line's pull-up/down resistor options are represented by the following bit sequences:
    - 00: Pull-up/down disable
    - 01: Pull-up
    - 10: Pull-down
    - 11: Reserved

    Pass the selected option in 2 least significant bits of `pull_up_down` parameter.

    Example `gpio_set_pull_up_down` call for enabling PD6 i/o line's pull-up resistor.  
    `gpio_set_pull_up_down(&ctx, 0b01000110, 0b01)`

5.  - Only with output i/o line function

        Set i/o line's value with `void gpio_set_value(gpio_context_t *ctx, uint8_t gpio, uint8_t value)`.

        Example `gpio_set_value` call for writing bit value 0 to PC10 i/o line.  
        `gpio_set_value(&ctx, 0b00101010, 0b0)`

    - Only with input i/o line function

        Get i/o line's value with `uint8_t gpio_get_value(gpio_context_t *ctx, uint8_t gpio)`.

        Example `gpio_get_value` call for reading bit value from PE6 i/o line.  
        `gpio_get_value(&ctx, 0b01000110)`

        The read bit value is placed in the least significant bit of 8-bit variable returned by `gpio_get_value`.

6. Release resources used by `gpio_context_t` struct instance with `int gpio_cleanup(gpio_context_t *ctx)`.

### Running
1. Compile your program with `gcc` without linking
    ```
    gcc program.c -c -o program.o
    ```

2. Link your program's object file `program.o` against compiled GPIO static library archive `libgpio.a`. After `-L` parameter, pass the name of the directory where you have `libgpio.a`.
    ```
    gcc program.o -L[directory] -l:libgpio.a -o program
    ```

3. Run the executable.
    ```
    sudo ./program
    ```
    It must be run with root's permissions because GPIO library operates on low-level registers accessible by CPU's address space. Otherwise `mmap` fails and `gpio_context_t` struct instance can not be initialized.

### Usage examples
Code for examples shown below is located in `examples` directory.
#### 1. Button
To build this example, use `make`
```
make button
```

In this example, a button acts as SPST (Single Pole Single Throw) switch. When the button is pressed, it shorts GPIO pin 38 (i/o line PC10) to ground and `gpio_get_value` returns 0. Due to enabled pull-up resistor mode, when the button is released, the i/o line PC10 is charged up to high logic level electric potential and `gpio_get_value` returns 1.

[<img src="https://i.imgur.com/QTu5XP3.png" width="500"/>](https://youtu.be/z5VB0dxBXFs)

(Click the image to watch the presentation on YouTube.)

#### 2. Display
This example uses SegDisp library dependent on GPIO library. SegDisp library can be used to drive a 4-digit 8-segment LED display module based on two 75HC595 shift registers connected in series (images below).

<img src="https://i.imgur.com/5xTmh8k.jpg" width="500"/>

<img src="https://i.imgur.com/OJsXFlm.jpg" width="500"/>

To build SegDisp, use
```
make libsegdisp
```

or (the same as for GPIO library in `Building` paragraph)

```
chmod 755 build.sh
./build.sh
```

Either way, compiled static library archive `libsegdisp.a` is placed in `./build` directory.

To build and run a program using SegDisp, use
```
gcc program.c -c -o program.o
gcc program.o -L[directory containing libsegdisp.a] -l:libsegdisp.a -o program
sudo ./program
```

You can also build `display` example with `make`
```
make display
```

This example presents how SegDisp library drives a display module. For details on data format accepted by the module, see `segdisp_display_digit` function definition in `segdisp.c` file.

[<img src="https://i.imgur.com/qMRmQco.png" width="500"/>](https://youtu.be/jkWAZQpNkA8)

The counter can be slowed down but refresh rate must be preserved to avoid flickering.

[<img src="https://i.imgur.com/HlrkiFj.png" width="500"/>](https://youtu.be/dpd_Jnp3cyA)

`base` parameter value passed to `segdisp_display_number` determines the numeral system in which the counter value is expressed.

[<img src="https://i.imgur.com/6gZzmLU.png" width="500"/>](https://youtu.be/im4N6AtdCXk)
