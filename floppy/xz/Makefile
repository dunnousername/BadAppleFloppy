.PHONY: all
OBJS = xz_dec_lzma2.elf xz_dec_stream.elf xz_crc32.elf
all: $(OBJS)

%.elf: %.c
	$(CC) $(CFLAGS) -static -nostdlib -ffreestanding -march=i486 -m32 -I../../include -c $< -o $@

clean:
	rm -f $(OBJS)