#define BUILD_IMAGE

#include <stdint.h>
#include "common.h"
#include "memfuncs.h"

#include "xz.h"
#include "decode.h"
#include "vga.h"

__attribute__((cdecl)) extern void _debug_msg(char *msg);
__attribute__((cdecl)) extern uint32_t get_esp(void);
__attribute__((cdecl)) extern void do_sti(void);
__attribute__((cdecl)) extern void do_cli(void);
__attribute__((cdecl)) extern void _halt(void);
__attribute__((cdecl)) extern void change_video_mode(uint32_t mode);
__attribute__((cdecl)) extern uint8_t *play_midi(uint8_t *ptr);

extern char _binary_blob_bin_start[];
#define _binary_chain_deflate_bin_start _binary_blob_bin_start
extern char _binary_blob_bin_end[];
#define _binary_chain_deflate_bin_end _binary_blob_bin_end

extern uint16_t midi_note_to_freq_table[];
extern uint8_t ba_music[];
extern uint8_t ba_music_end[];
extern uint32_t ba_music_len;

#define CHAIN_DEFLATE_BIN_START ((uint8_t *) _binary_chain_deflate_bin_start)
#define CHAIN_DEFLATE_BIN_END ((uint8_t *) _binary_chain_deflate_bin_end)
#define CHAIN_DEFLATE_BIN_LENGTH (((long) CHAIN_DEFLATE_BIN_END) - ((long) CHAIN_DEFLATE_BIN_START))

void panic(void);
void panic_msg(char *msg);
void write_hex(uint8_t value);
void timer_event(void);
void keyboard_event(void);
void update_frame(void);
void debug_msg(char *msg);
void info_msg(char *msg);
void print_stack(void);
static void try_deflate();
static int process_deflate_error(int ret);

#define VIDEO_MEMORY_START ((uint8_t*)0xB8000)
#define VIDEO_MEMORY_END (VIDEO_MEMORY_START+((320*200)))

#define FLOPPY_START ((uint8_t*) 0x100000)
#define FLOPPY_END (FLOPPY_START+2*80*18*512)

#define MEGABYTE (1024 * 1024)

#define SCRATCH_START FLOPPY_END
#define SCRATCH_SIZE (MAX_X * MAX_Y)
#define SCRATCH_END (SCRATCH_START + SCRATCH_SIZE)
#define SCRATCH_PTR (SCRATCH_START)
#define SCRATCH_PTR2 (SCRATCH_END)
#define SCRATCH2_END (SCRATCH_PTR2 + SCRATCH_SIZE)

#define DECOMPRESSED_START SCRATCH2_END
#define DECOMPRESSED_SIZE (MEGABYTE * 12)
#define DECOMPRESSED_END (DECOMPRESSED_START + DECOMPRESSED_SIZE)

uint8_t *music_ptr = ba_music;

static int initialized = 0;
static decoder_state_t state;
static int in_intro = 1;
volatile int update_time = 0;

/* #define DEBUG */

#define BAD_APPLE \
"The \"Bad Apple!!\" animation used in this demo may not be subject to the\n"\
"same license as the executable portion of this software.\n"\
"The use of this animation constitutes Fair Use since this program\n"\
"is of solely nonprofit and educational nature, since this program adds\n"\
"additional substance to the work used, and since the following notice is\n"\
"present in any unmodified source or binary version of this software,\n"\
"and in documentation that accompanies this project, which identifies\n"\
"and properly attributes the authors of the works from which this work\n"\
"is derived:\n"\
"\n"\
"The \"Bad Apple!!\" animation rendered by this application is the property\n"\
"of Team Shanghai Alice (original music), Alstroemeria Records (audio), and\n"\
"Nico Nico Douga user Anira (animation).\n"\
"A copy of the work can be found at https://www.youtube.com/watch?v=9lNZ_Rnr7Jc.\n"\
"\n"

#define LEO \
"A PC-speaker version of the Alstroemeria Records version of \"Bad Apple!!\"\n"\
"by Leonardo Ono is used with permission. The original code can be found at\n"\
"https://github.com/leonardo-ono/VgaTestesNotas.\n"\
"\n"

#define COPYRIGHT \
"The rights of the executable portion of source and binary distributions\n"\
"of this software are described in full in the following license information.\n"\
"This software falls under the MIT license. Attached below is a copy of the\n"\
"MIT license in full.\n"\
"Copyright (c) 2020 dunnousername (Henry Leonard) and contributors\n"\
"\n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy\n"\
"of this software and associated documentation files (the \"Software\"), to deal\n"\
"in the Software without restriction, including without limitation the rights\n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"\
"copies of the Software, and to permit persons to whom the Software is\n"\
"furnished to do so, subject to the following conditions:\n"\
"\n"\
"The above copyright notice and this permission notice shall be included in all\n"\
"copies or substantial portions of the Software.\n"\
"\n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"\
"SOFTWARE.\n"\
"\n"

#ifndef BAD_APPLE
#define BAD_APPLE ""
#endif

#ifndef LEO
#define LEO ""
#endif

#define VERSION "1.1.0 beta"
#define VERSION_TEXT "The version string of this software is \"" VERSION "\".\n"
#define STARTUP_NOTICE (BAD_APPLE LEO COPYRIGHT VERSION_TEXT)

__attribute__((cdecl)) void kernel_main() {
    debug_msg("Entered kernel_main()");
    info_msg(STARTUP_NOTICE);

    set_text_mode();
    char *msg = STARTUP_NOTICE;
    
    while (msg) {
        do_cli();
        clear_text();
        msg = write_text(msg);
        pause();
        do_sti();
        while (!update_time) _halt();
        update_time--;
    }

    do_cli();

    set_graphics_mode();

    try_deflate();

    music_ptr = ba_music;
    state.beginptr = DECOMPRESSED_START;
    state.state = STATE_START;
    initialized = 1;
    in_intro = 0;
    update_time = 0;

    debug_msg("rendering frames");

    /* makes debugging easier to do it now */
    do_sti();
    
    while (1) {
        while (!update_time) _halt();
        update_time--;
        update_frame();
        if (update_time >= 10) {
            info_msg("warning: video is running behind");
        }
    }

    debug_msg("reached end of kernel_main()");
    panic();
}

void print_stack() {
    info_msg("esp:");
    print_u32h(get_esp());
}

void debug_msg(char *msg) {
#ifdef DEBUG
    _debug_msg(msg);
#endif
}

void info_msg(char *msg) {
    _debug_msg(msg);
}

void draw_vertical_line(uint8_t *p, int16_t y1, int16_t y2, int16_t x) {
    if (y1 > y2) {
        return draw_vertical_line(p, y2, y1, x);
    }

    while (y1++ != y2) {
        p[sidx(x, y1)] = 0xFF;
    }
}

void draw_line(uint8_t *p, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (x1 > x2) {
        return draw_line(p, x2, y2, x1, y1);
    }

    while (x1++ != x2) {
        p[sidx(x1, y1)] = 0xFF;
    }

    draw_vertical_line(p, y1, y2, x1);
}

void update_frame() {
    if (!initialized) {
        return;
    }

    debug_msg("update_frame");
    
    char frame_msg[20] = "frame = ";
    kstrdec(frame_msg, state.frame);
    debug_msg(frame_msg);

    render(SCRATCH_PTR, &state);

    copy_to_vmem(SCRATCH_PTR);

    if (state.frame == MAX_FRAME) {
        music_ptr = ba_music;
    }

    return;
}

static int auto_step = 1;

static int _timer_counter = 0;
static int _timer_counter2 = 0;
void timer_event() {
    _timer_counter++;
    _timer_counter2++;

    if (_timer_counter == 1841) {
        _timer_counter = 0;
        if (auto_step && !in_intro && initialized) {
            update_time++;
        }
    }

    if (_timer_counter2 == /* 1767 */ 294) {
        _timer_counter2 = 0;
        if (initialized) {
            print_u32h((uint32_t) music_ptr);
            music_ptr = play_midi(music_ptr);
        }
    }
}

void keyboard_event() {
    debug_msg("received keyboard event");
    if (in_intro) {
        update_time++;
    } else {
#ifdef DEBUG
        if (auto_step) {
            auto_step = 0;
            update_time = 0;
        }
        update_time++;
#endif
    }

}

void try_deflate() {
    info_msg("Attempting decompression...");
    struct xz_buf buf;
    enum xz_ret ret = xz_decompress(
        CHAIN_DEFLATE_BIN_START,
        CHAIN_DEFLATE_BIN_LENGTH,
        DECOMPRESSED_START,
        DECOMPRESSED_SIZE,
        MEGABYTE * 2,
        &buf
    );

    char* codes[] = {
        "XZ_OK",
        "XZ_STREAM_END",
        "XZ_UNSUPPORTED_CHECK",
        "XZ_MEM_ERROR",
        "XZ_MEMLIMIT_ERROR",
        "XZ_FORMAT_ERROR",
        "XZ_OPTIONS_ERROR",
        "XZ_DATA_ERROR",
        "XZ_BUF_ERROR"
    };

    if (ret == XZ_STREAM_END) {
        info_msg("Decompression was successful.");
        return;
    }
    
    info_msg("Decompression not successful. Error code:");
    info_msg(codes[ret]);

    panic_msg("Decompression failed.");
}

void panic() {
    debug_msg("panic() was called; aborting");
    while (1) {
        /* die a slow, painful death... */
    }
}

void panic_msg(char *msg) {
    info_msg("Panicking! Reason:");
    info_msg(msg);
    panic();
}

int process_deflate_error(int ret) {
    if (ret == 0) {
        debug_msg("deflate completed successfully");
        return 0;
    }

    debug_msg("deflate has nonzero return code");

    if (ret == 1) {
        info_msg("output space exhausted before completing inflate");
        return ret;
    }

    char *negative_deflate_errors[] = {
        "invalid block type (type == 3)",
        "stored block length did not match one's complement",
        "dynamic block code description: too many length or distance codes",
        "dynamic block code description: code lengths codes incomplete",
        "dynamic block code description: repeat lengths with no first length",
        "dynamic block code description: repeat more than specified lengths",
        "dynamic block code description: invalid literal/length code lengths",
        "dynamic block code description: invalid distance code lengths",
        "dynamic block code description: missing end-of-block code",
        "invalid literal/length or distance code in fixed or dynamic block",
        "distance is too far back in fixed or dynamic block"
    };

    if ((ret < 0) && (ret > -12)) {
        info_msg(negative_deflate_errors[-ret - 1]);
        return ret;
    }

    info_msg("unknown deflate error");

    return ret;
}

void write_hex(uint8_t value) {
    char *tmp = "0x00";
    uint8_t ms = value >> 4;
    uint8_t ls = value & 0xf;
    if (ms < 10) {
        tmp[2] = '0' + ms;
    } else {
        tmp[2] = 'a' + (ms - 0xa);
    }

    if (ls < 10) {
        tmp[3] = '0' + ls;
    } else {
        tmp[3] = 'a' + (ls - 0xa);
    }

    debug_msg(tmp);
}