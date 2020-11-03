.PHONY: all clean
all: blob.elf

blob.elf:
	$(OBJCOPY) -I binary -O elf32-i386 blob.bin blob.elf

clean:
	rm -f blob.elf