#include "ops.h"
#include "common.h"
#include "constants.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void expand(uint8_t *input, int *output) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        output[i] = input[i];
    }
}

void quantize_down(int *input, uint8_t *output) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        output[i] = input[i] > 127 ? 255 : 0;
    }
}

void quantize_color(int *input, uint8_t *output) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        output[i] = input[i] & 255;
    }
}

static bool _replace(int *start, int *end, int a, int b) {
    bool ret = false;
    for (; start < end; start++) {
        if (*start == a) {
            *start = b;
            ret = true;
        }
    }
    return ret;
}

/* TODO: measure internal boundaries */
int label(int *input, int *output) {
    memset(output, 0, sizeof(int) * FRAME_SIZE);
    int counter = 1;
    int north = -MAX_X;
    int northwest = north - 1;
    int northeast = north + 1;
    int west = -1;

    for (int y = 0; y < MAX_Y; y++) {
        for (int x = 0; x < MAX_X; x++) {
            /* https://homepages.inf.ed.ac.uk/rbf/HIPR2/label.htm */
            if (input[y * MAX_X + x]) {
                int label = 0;
                if (x && y) {
                    if (output[northwest]) {
                        if (label) {
                            if (label != output[northwest]) {
                                _replace(output, &output[y * MAX_X + x], output[northwest], label);
                            }
                        }

                        label = output[northwest];
                    }
                }

                if (y) {
                    if (output[north]) {
                        if (label) {
                            if (label != output[north]) {
                                _replace(output, &output[y * MAX_X + x], output[north], label);
                            }
                        }

                        label = output[north];
                    }
                }

                if ((x < MAX_X - 1) && y) {
                    if (output[northeast]) {
                        if (label) {
                            if (label != output[northeast]) {
                                _replace(output, &output[y * MAX_X + x], output[northeast], label);
                            }
                        }

                        label = output[northeast];
                    }
                }

                if (x) {
                    if (output[west]) {
                        if (label) {
                            if (label != output[west]) {
                                _replace(output, &output[y * MAX_X + x], output[west], label);
                            }
                        }

                        label = output[west];
                    }
                }

                if (label == 0) {
                    label = -counter++;
                }

                output[y * MAX_X + x] = label;
            }
            north++; northwest++; northeast++; west++;
        }
    }

    int counter2 = 1;
    for (int i = 1; i <= counter; i++) {
        if (_replace(output, &output[FRAME_SIZE], -i, counter2)) {
            counter2++;
        }
    }

    return counter2;
}

void isolate(int *input, int *output, int label) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        output[i] = input[i] == label ? label : 0;
    }
}

/* http://www.cs.unca.edu/~reiser/imaging/chaincode.html */
static int16_t direction_dx[8] = {-1,  0,  1,  1,  1,  0, -1, -1};
static int16_t direction_dy[8] = {-1, -1, -1,  0,  1,  1,  1,  0};

static bool _is_edge(int *labeled, int region, int i) {
    int above = i - MAX_X;
    if (above >= 0) {
        if (labeled[above] != region) {
            return true;
        }
    }

    int below = i + MAX_X;
    if (above < FRAME_SIZE) {
        if (labeled[below] != region) {
            return true;
        }
    }

    int left = i - 1;
    if ((left >= 0) && (left % MAX_X != (MAX_X - 1))) {
        if (labeled[left] != region) {
            return true;
        }
    }

    int right = i + 1;
    if ((right < FRAME_SIZE) && (right % MAX_X != 0)) {
        if (labeled[right] != region) {
            return true;
        }
    }

    return false;
}

static bool _is_in_roi(int *input, int *x, int *y, uint8_t direction) {
    int dx = direction_dx[direction];
    int dy = direction_dy[direction];
    int xdx = *x + dx;
    int ydy = *y + dy;
    if ((xdx < 0) || (ydy < 0) || (xdx >= MAX_X) || (ydy >= MAX_Y)) {
        /* out of bounds, so we're not in the region of interest */
        return false;
    }

    if (input[ydy * MAX_X + xdx]) {
        /* we're in the region of interest */
        *x = xdx;
        *y = ydy;
        return true;
    }

    /* we're not in the region of interest */
    return false;
}

size_t find_chain(int *input, uint8_t *chain, int *start_x, int *start_y) {
    /* http://www.cs.unca.edu/~reiser/imaging/chaincode.html */
    bool ok = false;
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (input[i]) {
            *start_x = i % MAX_X;
            *start_y = i / MAX_X;
            ok = true;
            break;
        }
    }

    if (!ok) {
        return 0;
    }

    size_t len = 0;

    int r = *start_y;
    int c = *start_x;
    
    int direction = -1;

    for (int i = 0; i < 8; i++) {
        if (_is_in_roi(input, &c, &r, i)) {
            direction = i;
            chain[len++] = i;
            break;
        }
    }

    if (direction < 0) {
        return 0;
    }

    do {
        for (int j = 0; j < 8; j++) {
            int i = (j + direction + 5) % 8;
            if (_is_in_roi(input, &c, &r, i)) {
                direction = i;
                chain[len++] = i;
                break;
            }
        }
    } while ((r != *start_y) || (c != *start_x));

    return len;
}

uint8_t *find_chain2(int *input, int *start_x, int *start_y, size_t *len) {
    uint8_t chain[16384];
    size_t s = find_chain(input, chain, start_x, start_y);
    *len = s;
    if (!s) {
        return NULL;
    }
    uint8_t *out = malloc(s);
    memcpy(out, chain, s);
    return out;
}

static int _write_optimized_chains(chain_t *chains, uint8_t *chain, int start_x, int start_y, size_t len) {
    bool *okay_arr = malloc(sizeof(bool) * len);
    int *xs = malloc(sizeof(int) * len);
    int *ys = malloc(sizeof(int) * len);
    int x = start_x;
    int y = start_y;

    for (int i = 0; i < len; i++) {
        okay_arr[i] = false;
    }

    for (int i = 0; i < len; i++) {
        xs[i] = x;
        ys[i] = y;

        x += direction_dx[chain[i]];
        y += direction_dy[chain[i]];
        
        bool okay = (x != 0) && (x != MAX_X - 1) && (y != 0) && (y != MAX_Y - 1);
        okay_arr[i] = okay_arr[i] || okay;

        if (i) {
            okay_arr[i - 1] = okay_arr[i - 1] || okay;
        }

        if (i < len - 1) {
            okay_arr[i + 1] = okay_arr[i + 1] || okay;
        }
    }

    int num_chains = 0;
    bool prev = false;
    for (int i = 0; i < len; i++) {
        if ((!prev) && okay_arr[i]) {
            num_chains++;
            chains[num_chains - 1].present = true;
            chains[num_chains - 1].start_x = xs[i];
            chains[num_chains - 1].start_y = ys[i];
            chains[num_chains - 1].chain = &chain[i];
            chains[num_chains - 1].len = 1;
        } else if (okay_arr[i]) {
            chains[num_chains - 1].len++;
        }
        prev = okay_arr[i];
    }

    /* chains[num_chains].present = false; */

    for (int i = 0; i < num_chains; i++) {
        uint8_t *tmp = malloc(chains[i].len);
        memcpy(tmp, chains[i].chain, chains[i].len);
        chains[i].chain = tmp;
    }

    free(chain);
    free(xs);
    free(ys);
    free(okay_arr);

    return num_chains;
}

chain_t *find_all_chains(int *labeled, int labeled_len) {
    /* don't even expect that many to be on the stack */
    chain_t chains[1024];
    int tmp[FRAME_SIZE];
    int num_chains = 0;
    for (int i = 1; i < labeled_len; i++) {
        isolate(labeled, tmp, i);
        int start_x = 0;
        int start_y = 0;
        size_t len = 0;
        uint8_t *chain = find_chain2(tmp, &start_x, &start_y, &len);
        if (chain) {
            num_chains += _write_optimized_chains(&chains[num_chains], chain, start_x, start_y, len);
            /*
            chains[num_chains].chain = chain;
            chains[num_chains].start_x = start_x;
            chains[num_chains].start_y = start_y;
            chains[num_chains].present = true;
            chains[num_chains].len = len;
            num_chains++;
            */
        }
    }

    chains[num_chains].present = false;

    chain_t *ret = malloc(sizeof(chain_t) * (num_chains + 1));
    memcpy(ret, chains, sizeof(chain_t) * (num_chains + 1));

    return ret;
}

bool differ(uint8_t *a, uint8_t *b) {
    return memcmp(a, b, FRAME_SIZE) ? true : false;
}

static bool _on_border(int i) {
    int x = i % MAX_X;
    int y = i / MAX_X;
    return (x == 0) || (x == MAX_X - 1) || (y == 0) || (y == MAX_Y - 1);
}

bool find_floodfill(int *labeled, int region, int *x, int *y) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (labeled[i] == region) {
            if (!_is_edge(labeled, region, i)) {
                if (!_on_border(i)) {
                    *x = i % MAX_X;
                    *y = i / MAX_X;
                    return true;
                }
            }
        }
    }

    return false;
}

void invert(uint8_t *input, uint8_t *output) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        output[i] = ~input[i];
    }
}

void jam(uint8_t *input, uint8_t *output, size_t len) {
    for (int i = 0; i < (len >> 1); i++) {
        uint8_t b = input[i << 1] << 4;
        b |= input[(i << 1) | 1] & 0x0f;
        *output++ = b;
    }

    if (len & 1) {
        *output++ = input[len - 1] << 4;
    }
}
