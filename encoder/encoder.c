#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "stubs.h"
#include "common.h"
#include "decode.h"
#include "ops.h"

static uint8_t *frames;
static uint8_t *output;

uint8_t *_write_chains(int *labeled, int labeled_len, uint8_t color, uint8_t *ptr) {
    chain_t *chains = find_all_chains(labeled, labeled_len);
    for (int i = 0; chains[i].present; i++) {
        *ptr++ = color;
        chain_t chain = chains[i];
        ptr = write_varint(ptr, chain.start_x);
        ptr = write_varint(ptr, chain.start_y);
        ptr = write_varint(ptr, chain.len);
        memcpy(ptr, chain.chain, chain.len);
        ptr += chain.len;
        free(chain.chain);
    }
    free(chains);
    return ptr;
}

uint8_t *write_chains(uint8_t *frame, uint8_t color, uint8_t *ptr) {
    int *tmp1 = malloc(sizeof(int) * FRAME_SIZE);
    int *tmp2 = malloc(sizeof(int) * FRAME_SIZE);
    expand(frame, tmp1);
    int labeled_len = label(tmp1, tmp2);
    ptr = _write_chains(tmp2, labeled_len, color, ptr);
    free(tmp1);
    free(tmp2);
    return ptr;
}

uint8_t *do_encoding(uint8_t *frame, size_t *len) {
    uint8_t inverted[FRAME_SIZE];
    uint8_t encoded[16384] = {0};
    uint8_t *ptr = encoded;
    invert(frame, inverted);
    ptr = write_chains(frame, 0xff, ptr);
    ptr = write_chains(inverted, 0x55, ptr);
    *ptr++ = 0;
    /*
    uint8_t points[1024*2] = {0};
    uint8_t *points_ptr = points;
    int num_points = 0;
    for (int i = 1; i < labeled_len; i++) {
        int x = 0;
        int y = 0;
        if (find_floodfill(tmp2, i, &x, &y)) {
            num_points++;
            points_ptr = write_varint(points_ptr, x);
            points_ptr = write_varint(points_ptr, y);
        }
    }

    ptr = write_varint(ptr, num_points);
    memcpy(ptr, points, points_ptr - points);
    ptr += points_ptr - points;
    */
    int total_len = ptr - encoded;
    uint8_t *out = malloc(total_len);
    memcpy(out, encoded, total_len);
    *len = total_len;
    return out;
    
}

bool validate_encoding(uint8_t *reference, uint8_t *encoded, uint8_t *output) {
    decoder_state_t s;
    memset(output, 0, FRAME_SIZE);
    s.beginptr = encoded;
    s.ptr = encoded;
    s.frame = 0;
    s.x = -1;
    s.y = -1;
    s.state = STATE_START;
    render(output, &s);
    return !differ(reference, output);
}

/* useful for debugging */
void just_label(uint8_t *frame, uint8_t *out) {
    int tmp1[FRAME_SIZE] = {0};
    int tmp2[FRAME_SIZE] = {0};
    expand(frame, tmp1);
    label(tmp1, tmp2);
    quantize_color(tmp2, out);
}

uint8_t *do_frame(uint8_t *frame, uint8_t *out, size_t *len) {
    /* just_label(frame, out); */
    uint8_t *encoded = do_encoding(frame, len);
    if (!validate_encoding(frame, encoded, out)) {
        printf("warning: frame was invalid. continuing anyway.\n");
    }
    return encoded;
}

int main(int argc, char **argv) {
    printf("chain\n");
    frames = malloc(MAX_FRAME * FRAME_SIZE);
    if (!frames) {
        printf("failed to allocate memory\n");
        return 1;
    }
    FILE *f = fopen("input.raw", "rb");
    fread(frames, FRAME_SIZE, MAX_FRAME, f);
    fclose(f);
    output = malloc(MAX_FRAME * FRAME_SIZE);
    
    uint8_t **encoded = malloc(sizeof(uint8_t *) * MAX_FRAME);
    size_t *encoded_len = malloc(sizeof(size_t) * MAX_FRAME);

    PARALLEL_FOR
    for (int i = 0; i < MAX_FRAME; i++) {
        encoded[i] = do_frame(&frames[i * FRAME_SIZE], &output[i * FRAME_SIZE], &encoded_len[i]);
        printf("done with frame %d\n", i);
    }

    FILE *h = fopen("encoded.bin", "wb");
    for (int i = 0; i < MAX_FRAME; i++) {
        fwrite(encoded[i], 1, encoded_len[i], h);
        free(encoded[i]);
    }
    fclose(h);
    free(encoded);
    free(encoded_len);

    FILE *g = fopen("output.raw", "wb");
    fwrite(output, FRAME_SIZE, MAX_FRAME, g);
    fclose(g);

    free(frames);
    free(output);
    return 0;
}