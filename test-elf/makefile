all:
	i686-elf-as --32 test.asm -o test.o
	i686-elf-gcc -ffreestanding -nostdlib -nostartfiles -c main.c -o main.o
	i686-elf-ld -m elf_i386 -T link.ld -o test.elf main.o

