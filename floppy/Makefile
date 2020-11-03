.PHONY: all clean blob second_stage puff
all: image.bin

CFLAGS := $(CFLAGS) -target i386-pc-none-elf

image.bin: kernel.bin boot.bin
	cat first_stage/boot.bin kernel.bin > image.bin
	truncate -s 1440K image.bin

boot.bin:
	$(MAKE) -C first_stage

kernel.bin: blob second_stage puff
	$(LD) --oformat binary -o kernel.bin \
		kernel/second.elf \
		kernel/ckernel.elf \
		kernel/music.elf \
		blob/blob.elf \
		puff/puff.elf \
		 -Ttext=0x100200 -e _start

blob:
	$(MAKE) -C blob

second_stage:
	CFLAGS="$(CFLAGS)" $(MAKE) -C kernel

puff:
	CFLAGS="$(CFLAGS)" $(MAKE) -C puff

clean:
	$(MAKE) -C first_stage clean
	$(MAKE) -C blob clean
	$(MAKE) -C kernel clean
	$(MAKE) -C puff clean
	rm -f kernel.bin image.bin
