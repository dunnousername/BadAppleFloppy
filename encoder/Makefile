.PHONY: all clean run
all: encoder

encoder:
	$(CC) $(CFLAGS) -I../include -o encoder encoder.c

run: encoder
	./encoder

clean:
	rm -f encoder