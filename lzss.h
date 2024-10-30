#ifndef LZSS_H
#define LZSS_H

#include <stdlib.h>
#include "bit_buffer.h"

typedef struct match
{
    int length;
    int offset;
} match;

void lzss_compress(uint8_t *sliding_window, int bf_size, uint8_t *out_buffer, bit_buffer *bb);
int lzss_decompress(uint8_t *in_buffer, int bf_size, uint8_t *out_buffer, bit_buffer *bb);
int get_lab_size(int wi, int sbs, int lhbs);
match get_match(int sb_size, int sb_index, uint8_t *search_buffer, uint8_t *lookahead_buffer, int lhb_c_s);
void handle_match(bit_buffer *bb, match *m, char c);
void shift_search_buffer(int sb_size, int sb_index, int window_shift, uint8_t *search_buffer, uint8_t *lookahead_buffer);
void shift_lookahead_buffer(int window_index, int bf_size, int lab_size, uint8_t *lookahead_buffer, uint8_t *sliding_window);

#endif