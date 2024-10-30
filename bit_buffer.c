#include <stdlib.h>
#include "bit_buffer.h"

void init_bit_buffer(uint8_t* buffer, bit_buffer *bb) {
    bb->buffer = buffer;
    bb->head = 0;
    if (!bb->bit_count) {
        bb->bit_count = 0;
        bb->bit_buffer = 0;
    }
};

void bb_write_bit(bit_buffer *bb, int bit) {
    bb->bit_count++;
    bb->bit_buffer <<= 1;
    bb->bit_buffer |= bit;
    if (bb->bit_count == 8) {
        bb->buffer[bb->head] = bb->bit_buffer;
        bb->head++;
        bb->bit_buffer = 0;
        bb->bit_count = 0;
    }
};

char bb_write_char(bit_buffer *bb, uint8_t c) {
    if (bb->bit_count == 0) {
        bb->buffer[bb->head] = c;
        bb->head++;
        return c;
    }
    char tmp = c >> bb->bit_count;
    tmp |= (bb->bit_buffer << (8 - bb->bit_count));
    bb->bit_buffer = c;
    bb->buffer[bb->head] = tmp;
    bb->head++;
    return tmp;
};


void bb_write_tuple(bit_buffer *bb, tuple *pt) {
    uint8_t *bytes = (uint8_t*)pt;
    for (int i = 0; i < 3; i++) {
        bb_write_char(bb, bytes[i]);
    }
};

void bb_write_remaining(bit_buffer *bb) {
    while (bb->bit_count > 0) {
        bb_write_bit(bb, 0);
    }
}

int bb_get_bit(bit_buffer *bb) {
    if (bb->bit_count == 0) {
        bb->bit_buffer = bb->buffer[bb->head];
        bb->head++;
        bb->bit_count = 8;
    }
    uint8_t tmp = bb->bit_buffer >> (bb->bit_count -1);
    bb->bit_count--;
    return tmp & 1;
}

unsigned char bb_get_byte(bit_buffer *bb) {
    unsigned char tmp = bb->bit_buffer << (8 - bb->bit_count);
    bb->bit_buffer = bb->buffer[bb->head];
    bb->head++;
    tmp |= bb->bit_buffer >> bb->bit_count;
    return tmp;
}

tuple bb_get_tuple(bit_buffer *bb) {
    tuple t;
    t.offset = bb_get_byte(bb) | (bb_get_byte(bb) << 8);
    t.length = bb_get_byte(bb);
    return t;
}