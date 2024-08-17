file build/DaisyX7.elf
target extended-remote | openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c 'gdb_port pipe'
