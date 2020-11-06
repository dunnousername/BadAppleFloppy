.PHONY: all clean run distclean test
all: encoded.bin

FFMPEG ?= ffmpeg
OBJS := encoder.o ops.o

encoded.bin: run

input.raw:
	$(FFMPEG) -i assets/input.mkv -to 5:16 \
		-f lavfi -i color=gray:s=320x200 \
		-f lavfi -i color=black:s=320x200 \
		-f lavfi -i color=white:s=320x200 \
		-filter_complex \
		"[0:v]fps=24,scale=320:200[x]; \
		 [x][1:v][2:v][3:v]threshold[out]" \
		-map "[out]" \
		-c:v rawvideo -f rawvideo -pix_fmt gray \
		input.raw

%.o: %.c
	$(CC) $(CFLAGS) -fopenmp -glldb -g3 -I../include -c $< -o $@

encoder: $(OBJS)
	$(CC) $(CFLAGS) -fopenmp -glldb -g3 -o encoder $^

run: input.raw encoder
	./encoder

test: output.raw run
	ffplay -f rawvideo -vcodec rawvideo \
		-pixel_format bgr8 -video_size 320x240 \
		-framerate 24 \
		output.raw

clean:
	rm -f encoder $(OBJS) encoded.bin output.raw

distclean: clean
	rm -f input.raw