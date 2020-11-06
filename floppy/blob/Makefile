.PHONY: all clean
all: blob.elf

blob.bin:
	python3 generate.py

blob.elf: blob.bin
	$(OBJCOPY) -I binary -O elf32-i386 blob.bin blob.elf

clean:
	rm -f blob.elf