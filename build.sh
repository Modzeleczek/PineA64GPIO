#!/bin/bash

BUILD=./build

if [ -d $BUILD ]
then
  rm -r $BUILD
fi
mkdir -p $BUILD

SOURCE=./source
INCLUDE=-I./include
gcc $SOURCE/gpio.c $INCLUDE -c -o $BUILD/gpio.o
ar rcs $BUILD/libgpio.a $BUILD/gpio.o

gcc $SOURCE/segdisp.c $INCLUDE -c -o $BUILD/segdisp.o
ar rcs $BUILD/libsegdisp.a $BUILD/gpio.o $BUILD/segdisp.o

EXAMPLES=./examples
gcc $EXAMPLES/button.c $INCLUDE -c -o $BUILD/button.o
gcc $EXAMPLES/display.c $INCLUDE -c -o $BUILD/display.o

LINK=-L$BUILD
gcc $BUILD/button.o $LINK -l:libgpio.a -o $BUILD/button
gcc $BUILD/display.o $LINK -l:libsegdisp.a -o $BUILD/display
