.PHONY: all clean run distclean test
all: encoder

FFMPEG ?= ffmpeg
OBJS := encoder.o ops.o

input.raw:
	$(FFMPEG) -i assets/input.mkv \
		-f lavfi -i color=gray:s=320x240 \
		-f lavfi -i color=black:s=320x240 \
		-f lavfi -i color=white:s=320x240 \
		-filter_complex \
		"[0:v]fps=24,scale=320:240[x]; \
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

test: output.raw
	ffplay -f rawvideo -vcodec rawvideo \
		-pixel_format bgr8 -video_size 320x240 \
		-framerate 24 \
		output.raw

clean:
	rm -f encoder $(OBJS)

distclean: clean
	rm -f input.raw