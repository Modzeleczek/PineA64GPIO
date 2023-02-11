#!/bin/bash

dir=build

if [ -d $dir ]; then
  rm -r $dir
fi
mkdir $dir

gcc gpio.c -c -o $dir/gpio.o
ar rcs $dir/libgpio.a $dir/gpio.o

gcc gpio.c -c -o $dir/gpio.o
gcc segdisp.c -c -o $dir/segdisp.o
ar rcs $dir/libsegdisp.a $dir/gpio.o $dir/segdisp.o

ex=examples
gcc $ex/button.c -c -o $dir/button.o
gcc $dir/button.o -L$dir -l:libgpio.a -o $dir/button

gcc $ex/display.c -c -o $dir/display.o
gcc $dir/display.o -L$dir -l:libsegdisp.a -o $dir/display
