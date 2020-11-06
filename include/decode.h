#ifndef DECODE_H
#define DECODE_H

#include "constants.h"

#include <stdint.h>
#include "memfuncs.h"

#define REAL_MAX_Y (200)

#define IDX(x,y) ((y)*MAX_X+(x))
#define IDX2(x,y) ((y)*MAX_X*2+(x))

extern void panic_msg(char *msg);
extern void info_msg(char *msg);

void floodfill(uint8_t *p, int16_t x, int16_t y);

/*
 * safe index
 */
long sidx(long x, long y) {
    #ifdef BUILD_IMAGE
    if ((x >= MAX_X) || (x < 0) || (y >= MAX_Y) || (y < 0)) {
        panic_msg("2D array index not in range");
    }
    #endif

    return IDX(x, y);
}

uint8_t *read_varint(uint8_t *ptr, int16_t *out) {
    uint8_t lsb = *ptr++;
    if (lsb & 0b01000000) {
        int sign = lsb & 128;
        uint16_t msb = (uint16_t) *ptr++;
        int16_t us = (msb << 6) | (lsb & 63);
        *out = sign ? -us : us;
    } else {
        *out = lsb & 63;
    }

    return ptr;
}

uint8_t *write_varint(uint8_t *ptr, int16_t in) {
    if ((in >= 0) && (in <= 63)) {
        *ptr++ = in & 63;
        return ptr;
    }

    uint8_t flags = 64;
    if (in < 0) {
        flags |= 128;
        in = -in;
    }

    *ptr++ = flags | (in & 63);
    *ptr++ = (in >> 6) & 0xff;
    return ptr;
}

/*
 * move an array of eight-bit 0/255 values into an array of two-bit 0/3
 * values. every four bytes get turned into one during the transformation.
 */
void eight2two(uint8_t *src, uint8_t *dest, long len) {
    for (int i = 0; i < (len >> 2); i++) {
        uint8_t tmp = 0;
        tmp |= src[(i << 2) + 0] & 0b11000000;
        tmp |= src[(i << 2) + 1] & 0b00110000;
        tmp |= src[(i << 2) + 2] & 0b00001100;
        tmp |= src[(i << 2) + 3] & 0b00000011;
        dest[i] = tmp;
    }
}

#define VRAM_SKIP (MAX_X * 20)
#define VRAM_ADDR ((uint8_t *) 0xB8000)

void copy_to_vmem(uint8_t *src) {
    eight2two(src + VRAM_SKIP, src, MAX_X * REAL_MAX_Y);
    for (int i = 0; i < REAL_MAX_Y; i++) {
        if (i & 1) {
            memcpy(&VRAM_ADDR[((i & 0xfe) * MAX_X) >> 3] + 8192, &src[(i * MAX_X) >> 2], MAX_X>>2);
        } else {
            memcpy(&VRAM_ADDR[((i & 0xfe) * MAX_X) >> 3], &src[(i * MAX_X) >> 2], MAX_X>>2);
        }
    }
}

/* http://www.cs.unca.edu/~reiser/imaging/chaincode.html */
static int16_t direction_dx[8] = {-1,  0,  1,  1,  1,  0, -1, -1};
static int16_t direction_dy[8] = {-1, -1, -1,  0,  1,  1,  1,  0};

/*
 * move in an encoded direction
 */
void move_direction(int16_t *x, int16_t *y, char dir) {
    dir &= 0xf;
    
    /* 0000 NSWE */

    /*
    #define NORTH(a) ((a) & 8)
    #define SOUTH(a) ((a) & 4)
    #define  WEST(a) ((a) & 2)
    #define  EAST(a) ((a) & 1)

    int16_t dx = WEST (dir) ? -1 : (EAST (dir) ? 1 : 0);
    int16_t dy = NORTH(dir) ? -1 : (SOUTH(dir) ? 1 : 0);
    */

    int16_t dx = direction_dx[dir];
    int16_t dy = direction_dy[dir];

    *x += dx;
    *y += dy;

    /*
    #undef NORTH
    #undef SOUTH
    #undef WEST
    #undef EAST
    */
}

#define STATE_START        0
#define STATE_INPATH       1
#define STATE_ENDPATH      2
#define STATE_ENDFRAME     3
#define STATE_BEGINFRAME   4
#define STATE_DONE         5

typedef struct decoder_state_t {
    uint8_t state;
    uint8_t prevstate;
    uint8_t *ptr;
    uint16_t frame;
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
    int16_t length;
    uint8_t *endptr;
    uint8_t *beginptr;
    uint8_t color;
} decoder_state_t;

uint8_t decoder(decoder_state_t *s) {
    s->prevstate = s->state;
    switch (s->state) {
        case STATE_START:;
            s->frame = 0;
            s->color = 0xFF;
            if (s->beginptr) {
                s->ptr = s->beginptr;
            } else {
                s->beginptr = s->ptr;
            }
            s->state = STATE_BEGINFRAME;
            break;
        case STATE_ENDFRAME:;
            if (s->frame == MAX_FRAME) {
                s->state = STATE_DONE;
                break;
            }
            s->state = STATE_BEGINFRAME;
            break;
        case STATE_BEGINFRAME:;
            s->state = STATE_ENDPATH;
            s->x = -1;
            s->y = -1;
            break;
        case STATE_ENDPATH:;
            uint8_t tmp = s->ptr[0];
            s->ptr = &s->ptr[1];
            if (tmp == 0x00) {
                s->state = STATE_ENDFRAME;/* STATE_FLOODFILL; */
                s->frame++;
                break;
            }
            s->color = tmp;
            s->ptr = read_varint(s->ptr, &s->x);
            s->ptr = read_varint(s->ptr, &s->y);
            s->dx = 0;
            s->dy = 0;
            int16_t len = 0;
            s->ptr = read_varint(s->ptr, &len);
            if (len) {
                s->state = STATE_INPATH;
                s->endptr = s->ptr + len;
            } else {
                s->endptr = (uint8_t *) 0;
            }
            break;
        case STATE_INPATH:;
            if (s->ptr == s->endptr) {
                s->state = STATE_ENDPATH;
                s->endptr = (uint8_t *) 0;
                break;
            }
            tmp = *(s->ptr)++;
            int16_t tmpx = s->x;
            int16_t tmpy = s->y;
            move_direction(&s->x, &s->y, tmp);
            s->dx = s->x - tmpx;
            s->dy = s->y - tmpy;
            break;
        case STATE_DONE:;
            s->state = STATE_START;
            break;
        /*
        case STATE_FLOODFILL:;
            s->ptr = read_varint(s->ptr, &s->length);
            s->state = STATE_INFLOODFILL;
            break;
        case STATE_INFLOODFILL:;
            if (s->length == 0) {
                s->frame++;
                s->state = STATE_ENDFRAME;
                break;
            }
            s->ptr = read_varint(s->ptr, &s->x);
            s->ptr = read_varint(s->ptr, &s->y);
            s->length--;
            break;
        */
    }

    return s->state;
}

void floodfill(uint8_t *p, int16_t x, int16_t y) {
    if ((x < 0) || (x >= MAX_X) || (y < 0) || (y >= MAX_Y)) {
        info_msg("warning: tried to floodfill a point that was not in image bounds");
        return;
    }

    if (p[sidx(x, y)] == 0xFF) {
        return;
    }

    while ((x >= 0) && !p[sidx(x, y)]) {
        x--;
    }
    x++;

    int16_t pointsx[32] = {0};
    int16_t pointsy[32] = {0};
    int pointslen = 0;
    int above = 0;
    int below = 0;
    int16_t end = x;
    for (int16_t i = x; (i < MAX_X) && !(p[sidx(i, y)] == 255); i++) {
        end = i;
        p[sidx(i, y)] = 0xFF;
    }
    end++;

    for (int16_t i = x; i < end; i++) {
        if (pointslen >= 30) {
            for (;pointslen > 0; pointslen--) {
                floodfill(p, pointsx[pointslen - 1], pointsy[pointslen - 1]);
            }
        }

        if ((y > 0) && !p[sidx(i, y - 1)]) {
            if (!above) {
                pointsx[pointslen] = i;
                pointsy[pointslen] = y - 1;
                above = 1;
                pointslen++;
            }
        } else {
            above = 0;
        }

        if ((y < MAX_Y - 1) && !p[sidx(i, y + 1)]) {
            if (!below) {
                pointsx[pointslen] = i;
                pointsy[pointslen] = y + 1;
                below = 1;
                pointslen++;
            }
        } else {
            below = 0;
        }
    }

    for (;pointslen; pointslen--) {
        floodfill(p, pointsx[pointslen - 1], pointsy[pointslen - 1]);
    }
}

void render(uint8_t *scratch_ptr, decoder_state_t *state) {
    memset(scratch_ptr, 0, FRAME_SIZE);

    int x = 0;
    int y = 0;

    while (state->state != STATE_BEGINFRAME) {
        decoder(state);
    }

    while (decoder(state) != STATE_BEGINFRAME) {
        if ((state->prevstate == STATE_ENDPATH) || (state->prevstate == STATE_INPATH)) {
            if ((state->x >= 0) && (state->y >= 0)) {
                scratch_ptr[sidx(state->x, state->y)] = state->color;
            }
        }
    }

    /*
    while (decoder(state) != STATE_ENDFRAME) {
        if (state->prevstate == STATE_INFLOODFILL) {
            #if 0
            debug_msg("flooding");
            floodfill(scratch_ptr, state->x, state->y);
            #endif
        }
    }
    */
}

#endif