rm -f *.o kernel.elf
make
qemu-system-riscv64 -machine virt -m 128M -kernel kernel.elf -nographic
