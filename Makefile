.PHONY: all clean floppy clean.floppy encoder clean.encoder run.encoder
all: floppy encoder

CFLAGS = $(CFLAGS) -target i386-pc-none-elf
LD ?= ld.lld
OBJCOPY ?= llvm-objcopy

floppy: run.encoder
	$(MAKE) -C floppy

clean.floppy:
	$(MAKE) -C floppy clean

encoder:
	$(MAKE) -C encoder

clean.encoder:
	$(MAKE) -C encoder clean

run.encoder: encoder
	$(MAKE) -C encoder run

clean: clean.floppy clean.encoder
