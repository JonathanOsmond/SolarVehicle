#!/usr/bin/env bash

# Move to script directory
cd "${0%/*}"

cgdb -d `which arm-none-eabi-gdb` --eval-command 'target extended-remote localhost:3333' --eval-command 'load' ./build/*.elf "$@"
