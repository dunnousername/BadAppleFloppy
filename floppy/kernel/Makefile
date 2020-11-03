.PHONY: all
all: second.elf music.elf ckernel.elf

second.elf:
	nasm -felf32 second.asm -o second.elf
music.elf:
	nasm -felf32 music.asm -o music.elf
ckernel.elf:
	$(CC) $(CFLAGS) -static -nostdlib -ffreestanding -march=i486 -m32 -I../../include -I../puff -o ckernel.elf -c ckernel.c

clean:
	rm -f second.elf music.elf ckernel.elf