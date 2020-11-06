.PHONY: all clean floppy test.encoder clean.floppy encoder clean.encoder run.encoder distclean.encoder
all: floppy

CFLAGS = $(CFLAGS) -target i386-pc-none-elf
LD ?= ld.lld
OBJCOPY ?= llvm-objcopy

floppy: $(if $(wildcard encoder/encoded.bin),,encoder/encoded.bin)
	$(MAKE) -C floppy

clean.floppy:
	$(MAKE) -C floppy clean

encoder/encoded.bin:
	$(MAKE) -C encoder

clean.encoder:
	$(MAKE) -C encoder clean

distclean.encoder:
	$(MAKE) -C encoder distclean

test.encoder:
	$(MAKE) -C encoder test

clean: clean.floppy clean.encoder

distclean: clean distclean.encoder
