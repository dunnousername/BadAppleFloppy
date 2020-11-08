.PHONY: all clean blob second_stage xz
all: image.bin

CFLAGS := $(CFLAGS) -target i386-pc-none-elf

image.bin: kernel.bin boot.bin
	cat first_stage/boot.bin kernel.bin > image.bin
	cp image.bin ../bin/floppy.img
	truncate -s 1440K ../bin/floppy.img

boot.bin:
	$(MAKE) -C first_stage

kernel.bin: kernel.elf
	$(OBJCOPY) -Obinary kernel.elf kernel.bin

kernel.elf: blob second_stage xz
	$(LD) -Ttext=0x100200 -e _start \
		-o kernel.elf \
		kernel/second.elf \
		kernel/ckernel.elf \
		kernel/music.elf \
		blob/blob.elf \
		xz/*.elf

blob:
	$(MAKE) -C blob

xz:
	CFLAGS="$(CFLAGS)" $(MAKE) -C xz

second_stage:
	CFLAGS="$(CFLAGS)" $(MAKE) -C kernel

clean:
	$(MAKE) -C first_stage clean
	$(MAKE) -C blob clean
	$(MAKE) -C kernel clean
	$(MAKE) -C xz clean
	rm -f kernel.bin image.bin
