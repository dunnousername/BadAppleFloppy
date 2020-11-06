.PHONY: all clean
all: blob.elf

blob.bin:
	#python generate.py || python3 generate.py
	rm -f ../../encoder/encoded.bin.xz
	xz -k -e -T0 -z --lzma2="dict=1MiB" \
		--memlimit-decompress=16MiB -9 \
		../../encoder/encoded.bin
	cp ../../encoder/encoded.bin.xz blob.bin
	
blob.elf: blob.bin
	$(OBJCOPY) -I binary -O elf32-i386 blob.bin blob.elf

clean:
	rm -f blob.elf