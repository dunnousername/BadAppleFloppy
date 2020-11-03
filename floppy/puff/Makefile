.PHONY: all clean
all: puff.elf

puff.elf:
	$(CC) $(CFLAGS) -static -nostdlib -ffreestanding -march=i486 -m32 -I../../include -o puff.elf -c puff.c

clean:
	rm -f puff.elf