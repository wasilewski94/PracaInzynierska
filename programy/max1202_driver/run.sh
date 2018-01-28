#!/bin/bash
rmmod timer.ko
make clean
make
gcc test.c -o complete_test.e
insmod timer.ko
echo "----------------------------------"
echo "------------TEST------------------"
echo "----------------------------------"
./complete_test.e

