CC = gcc
CFLAGS = -Wall -Wextra
BUILD = ./build

all: libgpio libsegdisp

libgpio: $(BUILD)/libgpio.a
$(BUILD)/libgpio.a: $(BUILD)/gpio.o
	ar rcs $@ $^
$(BUILD)/gpio.o: gpio.c gpio.h # w gpio.c jest #include "gpio.h"
	@mkdir -p $(BUILD)
	gcc $(word 1, $^) -c -o $@

libsegdisp: $(BUILD)/libsegdisp.a
$(BUILD)/libsegdisp.a: $(BUILD)/gpio.o $(BUILD)/segdisp.o
	ar rcs $@ $^
$(BUILD)/segdisp.o: segdisp.c segdisp.h
	@mkdir -p $(BUILD)
	gcc $(word 1, $^) -c -o $@

clean:
	@rm -rvf $(BUILD)
