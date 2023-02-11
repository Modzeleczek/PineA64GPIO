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
