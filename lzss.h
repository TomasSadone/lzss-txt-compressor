#ifndef LZSS_H
#define LZSS_H

#include <stdlib.h>

typedef struct bit_buffer {
    int head; //reference to the end of last whole byte written. (*buffer[head - 1] would access last byte)
    unsigned char bit_count;
    unsigned char bit_buffer;
    uint8_t* buffer;
} bit_buffer;

typedef struct tuple {
    uint16_t offset; //representa un valor de 1 - 32,768 bytes;
    uint8_t length; // representa un valor de 3 - 258 bytes;
    // https://github.com/MichaelDipperstein/lzss/blob/master/lzss.c linea 162 tener en cuenta para el length de 3 - 258
    // .. lo dice tambien en moddingwiki decoding procedure 
} tuple;

bit_buffer lzss_compress(uint8_t* sliding_window, int bf_size, uint8_t* out_buffer, unsigned char prev_bb_bit_count, unsigned char prev_bit_buffer);
int lzss_decompress(uint8_t *in_buffer, int bf_size, uint8_t *out_buffer);
int get_lab_size(int wi, int sbs, int lhbs);
bit_buffer init_bit_buffer(uint8_t* buffer);
int bb_get_bit(bit_buffer *bb);
unsigned char bb_get_byte(bit_buffer *bb);
tuple bb_get_tuple(bit_buffer *bb);
void bb_write_bit(bit_buffer *bb, int bit);
char bb_write_char(bit_buffer *bb, uint8_t c);
void bb_write_tuple(bit_buffer *bb, tuple *pt);
void bb_write_tuple(bit_buffer *bb, tuple *pt) ;
void bb_write_remaining(bit_buffer *bb);

#endif