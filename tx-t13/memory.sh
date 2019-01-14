avr-size   main.elf
avr-nm -Crtd --size-sort main.elf | grep -i ' [dbv] '
avr-objdump -j .data -s main.elf
