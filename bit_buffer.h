#ifndef BB_H
#define BB_H

#include <stdlib.h>

typedef struct tuple
{
    uint16_t offset;
    uint8_t length;
} tuple;

typedef struct bit_buffer
{
    int head; // reference to the end of last whole byte written. (*buffer[head - 1] would access last byte)
    unsigned char bit_count;
    unsigned char bit_buffer;
    uint8_t *buffer;
} bit_buffer;

void init_bit_buffer(uint8_t *buffer, bit_buffer *bb);
int bb_get_bit(bit_buffer *bb);
unsigned char bb_get_byte(bit_buffer *bb);
tuple bb_get_tuple(bit_buffer *bb);
void bb_write_bit(bit_buffer *bb, int bit);
char bb_write_char(bit_buffer *bb, uint8_t c);
void bb_write_tuple(bit_buffer *bb, tuple *pt);
void bb_write_tuple(bit_buffer *bb, tuple *pt);
void bb_write_remaining(bit_buffer *bb);
#endif