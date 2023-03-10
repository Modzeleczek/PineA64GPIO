CC = gcc
CFLAGS = -Wall -Wextra
BUILD = ./build
EXAMPLES = ./examples

all: libgpio libsegdisp button display

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

button: $(BUILD)/libgpio.a $(BUILD)/button.o
	gcc $(word 2,$^) -L$(BUILD) -l:$(notdir $(word 1,$^)) -o $(BUILD)/$@
$(BUILD)/button.o: $(EXAMPLES)/button.c
	@mkdir -p $(BUILD)
	gcc $^ -c -o $@

display: $(BUILD)/libsegdisp.a $(BUILD)/display.o
	gcc $(word 2,$^) -L$(BUILD) -l:$(notdir $(word 1,$^)) -o $(BUILD)/$@
$(BUILD)/display.o: $(EXAMPLES)/display.c
	@mkdir -p $(BUILD)
	gcc $^ -c -o $@

clean:
	@rm -rvf $(BUILD)
