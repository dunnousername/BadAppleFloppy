#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include "memfuncs.h"

__attribute__((cdecl)) extern void change_video_mode(uint32_t mode);

#define TEXT_MODE 0x03
#define GRAPHICS_MODE 0x04
#define UNINITIALIZED_MODE ((uint32_t) -1)

static uint32_t which_mode = UNINITIALIZED_MODE;

uint32_t get_video_mode() {
    return which_mode;
}

static int cursor_x = 0;
static int cursor_y = 0;

void set_text_mode() {
    change_video_mode(which_mode = TEXT_MODE);
    cursor_x = 0;
    cursor_y = 0;
}

void set_graphics_mode() {
    change_video_mode(which_mode = GRAPHICS_MODE);
}

#define TEXT_MODE_PTR ((uint8_t *) 0xB8000)

#define COLOR 0x3F

void clear_text() {
    if (get_video_mode() != TEXT_MODE) {
        return;
    }

    for (int i = 0; i < 80 * 25; i++) {
        TEXT_MODE_PTR[(i << 1) | 0] = 0;
        TEXT_MODE_PTR[(i << 1) | 1] = COLOR;
    }

    cursor_x = 0;
    cursor_y = 0;
}

char *write_text(char *msg) {
    if (get_video_mode() != TEXT_MODE) {
        set_text_mode();
    }

    if (!msg) {
        return (char *) 0;
    }

    while ((cursor_y < 24) && *msg) {
        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
            continue;
        }

        int c = *msg++;
        if (c == 0) {
            return (char *) 0;
        }

        if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
            continue;
        }

        int tmp = (cursor_y * 80 + cursor_x) << 1;
        TEXT_MODE_PTR[tmp | 0] = c;
        TEXT_MODE_PTR[tmp | 1] = COLOR;

        cursor_x++;
    }

    return *msg ? msg : (char *) 0;
}

void pause() {
    char *s = "Press any key to continue...";

    int tmp = (24 * 80) * 2;

    do {
        TEXT_MODE_PTR[tmp++] = *s;
        TEXT_MODE_PTR[tmp++] = 0x07;
    } while (*s++);
}

#endif