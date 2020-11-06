.PHONY: all
OBJS = xz_dec_lzma2.elf xz_dec_stream.elf xz_crc32.elf
all: $(OBJS)

%.elf: %.c
	$(CC) $(CFLAGS) -flto -ffreestanding -nostartfiles -I../../include -c $< -o $@

clean:
	rm -f $(OBJS)