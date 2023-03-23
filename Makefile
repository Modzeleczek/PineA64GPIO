MAKEFLAGS += --no-builtin-rules # Disable implicit rules execution.
BUILD = ./build
SOURCE = ./source
INCLUDE = ./include
EXAMPLES = ./examples

all: libgpio libsegdisp button display

define archive_library
	ar rcs $@ $^
endef

define build_object
	@mkdir -p $(BUILD)
	gcc $(word 1,$^) -I$(INCLUDE) -c -o $@
endef

libgpio: $(BUILD)/libgpio.a
$(BUILD)/libgpio.a: $(BUILD)/gpio.o
	$(call archive_library)
# gpio.c contains #include "gpio.h" so gpio.o depends on them both.
$(BUILD)/gpio.o: $(SOURCE)/gpio.c $(INCLUDE)/gpio.h
	$(call build_object)

libsegdisp: $(BUILD)/libsegdisp.a
$(BUILD)/libsegdisp.a: $(BUILD)/gpio.o $(BUILD)/segdisp.o
	$(call archive_library)
$(BUILD)/segdisp.o: $(SOURCE)/segdisp.c $(INCLUDE)/segdisp.h
	$(call build_object)

define build_example
	gcc $(word 2,$^) -L$(BUILD) -l:$(notdir $(word 1,$^)) -o $(BUILD)/$@
endef

button: $(BUILD)/libgpio.a $(BUILD)/button.o
	$(call build_example)
$(BUILD)/button.o: $(EXAMPLES)/button.c $(INCLUDE)/gpio.h
	$(call build_object)

display: $(BUILD)/libsegdisp.a $(BUILD)/display.o
	$(call build_example)
$(BUILD)/display.o: $(EXAMPLES)/display.c $(INCLUDE)/gpio.h $(INCLUDE)/segdisp.h
	$(call build_object)

clean:
	@rm -rvf $(BUILD)
