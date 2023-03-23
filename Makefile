MAKEFLAGS += --no-builtin-rules # Disable implicit rules execution.
BUILD = ./build
SOURCE = ./source
INCLUDE = ./include
EXAMPLES = ./examples

all: libgpio libsegdisp button display

libgpio: $(BUILD)/libgpio.a
$(BUILD)/libgpio.a: $(BUILD)/gpio.o
	ar rcs $@ $^
# gpio.c contains #include "gpio.h" so gpio.o depends on them both.
$(BUILD)/gpio.o: $(SOURCE)/gpio.c $(INCLUDE)/gpio.h
	@mkdir -p $(BUILD)
	gcc $(word 1,$^) -I$(INCLUDE) -c -o $@

libsegdisp: $(BUILD)/libsegdisp.a
$(BUILD)/libsegdisp.a: $(BUILD)/gpio.o $(BUILD)/segdisp.o
	ar rcs $@ $^
$(BUILD)/segdisp.o: $(SOURCE)/segdisp.c $(INCLUDE)/segdisp.h
	@mkdir -p $(BUILD)
	gcc $(word 1,$^) -I$(INCLUDE) -c -o $@

button: $(BUILD)/libgpio.a $(BUILD)/button.o
	gcc $(word 2,$^) -L$(BUILD) -l:$(notdir $(word 1,$^)) -o $(BUILD)/$@
$(BUILD)/button.o: $(EXAMPLES)/button.c $(INCLUDE)/gpio.h
	@mkdir -p $(BUILD)
	gcc $(word 1,$^) -I$(INCLUDE) -c -o $@

display: $(BUILD)/libsegdisp.a $(BUILD)/display.o
	gcc $(word 2,$^) -L$(BUILD) -l:$(notdir $(word 1,$^)) -o $(BUILD)/$@
$(BUILD)/display.o: $(EXAMPLES)/display.c $(INCLUDE)/gpio.h $(INCLUDE)/segdisp.h
	@mkdir -p $(BUILD)
	gcc $(word 1,$^) -I$(INCLUDE) -c -o $@

clean:
	@rm -rvf $(BUILD)
