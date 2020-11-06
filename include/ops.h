#ifndef OPS_H
#define OPS_H

#include <stdint.h>
#include <stdbool.h>
#include "constants.h"

/* expand the input from uint8_t to int */
void expand(uint8_t *input, int *output);

/* replace input with uint8_t (255) if > 127 else 0 */
void quantize_down(int *input, uint8_t *output);

/* quantize_down but make the value the lowest 8 bits */
void quantize_color(int *input, uint8_t *output);

/*
 * label regions with a unique number for each region
 * output is cleared
 */
int label(int *input, int *output);

/*
 * isolate a labeled region. output need not be clear.
 */
void isolate(int *input, int *output, int label);

/*
 * find chain codes of an isolated region
 */
size_t find_chain(int *input, uint8_t *chain, int *start_x, int *start_y);

/*
 * same thing but allocate a buffer for it
 * remember to free it later
 */
uint8_t *find_chain2(int *input, int *start_x, int *start_y, size_t *len);

typedef struct chain_t {
    int start_x;
    int start_y;
    uint8_t *chain;
    size_t len;
    bool present;
} chain_t;

/*
 * find all the chains
 * remember to free
 */
chain_t *find_all_chains(int *labeled, int labeled_len);

/*
 * true if the frames differ, false otherwise
 */
bool differ(uint8_t *a, uint8_t *b);

/*
 * find floodfill points for a region
 * if no points could be found, return false; else true
 */
bool find_floodfill(int *labeled, int region, int *x, int *y);

/*
 * invert an image
 */
void invert(uint8_t *input, uint8_t *output);

/*
 * jam a sequence of bytes so that the bottom nibbles of two consecutive
 * bytes are the two halves of one of the output bytes
 */
void jam(uint8_t *input, uint8_t *output, size_t len);

#endif