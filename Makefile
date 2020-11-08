.PHONY: all clean floppy test.encoder clean.floppy encoder clean.encoder
.PHONY: run.encoder distclean.encoder encoder qemu qemu-monitor qemu-gdb
.PHONY: lldb gdb
all: floppy

CFLAGS = $(CFLAGS) -target i386-pc-none-elf -Os
LD ?= ld.lld
OBJCOPY ?= llvm-objcopy

floppy: $(if $(wildcard encoder/encoded.bin),,encoder)
	$(MAKE) -C floppy

clean.floppy:
	$(MAKE) -C floppy clean

encoder:
	$(MAKE) -C encoder

clean.encoder:
	$(MAKE) -C encoder clean

distclean.encoder:
	$(MAKE) -C encoder distclean

test.encoder:
	$(MAKE) -C encoder test

clean: clean.floppy clean.encoder

distclean: clean distclean.encoder

qemu:
	qemu-system-i386 -debugcon stdio -m 64M -fda bin/floppy.img

qemu-monitor:
	qemu-system-i386 -monitor stdio -m 64M -fda bin/floppy.img

qemu-gdb:
	qemu-system-i386 -s -S -debugcon stdio -m 64M -fda bin/floppy.img

gdb:
	gdb floppy/kernel.elf

lldb:
	lldb floppy/kernel.elf
