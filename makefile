TARGET := kernel.elf
ISO := kernel.iso
CC := i686-elf-gcc
AS := i686-elf-as
LD := i686-elf-ld

CFLAGS := -g -std=gnu99 -ffreestanding -O0 -Wall -Wextra -m32 -I./boot -I./kernel -I./drivers -I./lib
ASFLAGS :=
LDFLAGS := -T boot/linker.ld -O0 -nostdlib -m elf_i386

C_SOURCES := $(shell find boot kernel drivers lib -name '*.c')
ASM_SOURCES := $(shell find boot kernel drivers lib -name '*.asm')

OBJ_C := $(patsubst %.c, obj/%.o, $(C_SOURCES))
OBJ_ASM := $(patsubst %.asm, obj/%.o, $(ASM_SOURCES))
OBJ := $(OBJ_C) $(OBJ_ASM)

all: $(TARGET)

$(OBJ): | objdirs


objdirs:
	mkdir -p $(dir $(OBJ))


obj/%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@


obj/%.o: %.asm
	@echo "[ASM] $<"
	@$(AS) $(ASFLAGS) -c $< -o $@


$(TARGET) : $(OBJ)
	@echo "[LD] Linking kernel ..."
	@$(LD) -m elf_i386 $(OBJ) $(LDFLAGS) -o $(TARGET)


iso: $(TARGET)
	@echo "[ISO] Building ISO..."
	mkdir -p isodir/boot/grub
	cp $(TARGET) isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $(ISO) isodir


run: iso $(ISO) 
	qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE.fd -cdrom $(ISO) -m 1024M -serial stdio -hda disk.img

debug : iso $(ISO) 
	qemu-system-i386 -cdrom $(ISO) -monitor stdio -no-reboot -no-shutdown -d cpu_reset -D qemu.log -s -S

gdb:
	gdb-multiarch $(TARGET)

clean:
	@echo "[CLEAN]"
	rm -rf obj $(TARGET) $(ISO)

.PHONY: all clean iso run objdirs
