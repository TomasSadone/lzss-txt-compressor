#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "./lzss.h"

#define SLIDING_WINDOW_SIZE (1024 * 32) //~32.000kb
#define LOOKAHEAD_BUFFER_SIZE 258 //bytes
#define SEARCH_BUFFER_SIZE (SLIDING_WINDOW_SIZE - LOOKAHEAD_BUFFER_SIZE) 

void print_binary_1(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (byte >> i) & 1);
    }
    printf(" ");
}

void print_binary_buffer_1(uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        print_binary_1(buffer[i]);
    }
    printf("\n");
}

void lzss_compress(uint8_t* sliding_window, int bf_size, uint8_t* out_buffer, bit_buffer *bb) {     
    int lab_size = LOOKAHEAD_BUFFER_SIZE > bf_size ? bf_size : LOOKAHEAD_BUFFER_SIZE;
    int sb_size = SEARCH_BUFFER_SIZE > bf_size ? bf_size : SEARCH_BUFFER_SIZE;
    uint8_t lookahead_buffer[lab_size];
    memcpy(&lookahead_buffer, sliding_window, lab_size);
    uint8_t search_buffer[sb_size];
    memset(search_buffer, 0, sb_size);
    // uint8_t *search_buffer = calloc(sb_size, sizeof(uint8_t));
    int window_index = 0;

    // bit_buffer bb = init_bit_buffer(out_buffer);
    init_bit_buffer(out_buffer, bb);
    
    while (true) {
        int match_length = 0;
        int match_offset = 0;
        for (int i = sb_size - window_index; i < sb_size; i++) {
            if(memcmp(&search_buffer[i], &lookahead_buffer[0], sizeof(uint8_t)) == 0) {
                int current_match_length = 1;
                int j = 1;
                int lhb_c_s = get_lab_size(window_index, sb_size, lab_size);
                while (j < lhb_c_s) {
                    uint8_t *current_buffer;
                    if (i + j < sb_size) {
                        current_buffer = &search_buffer[i + j];
                    } else {
                        current_buffer = &lookahead_buffer[i + j - sb_size];
                    }

                    if (memcmp(current_buffer, &lookahead_buffer[j], sizeof(uint8_t)) != 0) {
                        break;
                    }
                    ++current_match_length;
                    ++j;
                }
                if (current_match_length > match_length) {
                    match_length = current_match_length;
                    match_offset = sb_size - i;
                }
            }
            if (match_length + i + 1 > sb_size) break; //..+1 por la prox iteracion
        }

        //if there's no match, move window one position anyways.
        int window_shift = match_length <= 2 ? 1 : match_length;
        if (match_length > 2) {
            tuple node;
            node.offset = match_offset;
            node.length = match_length;
            bb_write_bit(bb, 1);
            bb_write_tuple(bb, &node);
        } else {
            bb_write_bit(bb, 0);
            char written_char = bb_write_char(bb, lookahead_buffer[0]);
        }
        
        if(window_index + window_shift >= sb_size) {
            break;
        }
        //Shift search_buffer
            //shift entire search buffer window_shift times
            memmove(&search_buffer[sb_size  - window_index - window_shift], &search_buffer[sb_size  - window_index], window_index);

            //fill last garbage bytes with firsts from lookahead
            memcpy(&search_buffer[sb_size - window_shift], &lookahead_buffer[0], window_shift);

       
        //Shift lookahead_buffer
        window_index += window_shift;    
        int next_lhbs =  get_lab_size(window_index, sb_size, lab_size);
        memmove(&lookahead_buffer, &sliding_window[window_index], next_lhbs);
        if (next_lhbs < lab_size) {
            memset(&lookahead_buffer[next_lhbs], 0, lab_size - next_lhbs);
        }
    }
}

int lzss_decompress(uint8_t *in_buffer, int bf_size, uint8_t *out_buffer, bit_buffer *bb) {
    init_bit_buffer(in_buffer, bb);
    int char_count = 0;
    while (bb->head < bf_size) {        
        if (bb_get_bit(bb) == 0) {
            out_buffer[char_count] = bb_get_byte(bb);;
            char_count++;
        } else {
            tuple t = bb_get_tuple(bb);
            for (int i = 0; i < t.length; i++) {
                out_buffer[char_count + i] = out_buffer[char_count + i - t.offset ];
            }
            memmove(&out_buffer[char_count], &out_buffer[char_count - t.offset], t.length);
            char_count += t.length;
        }
    }
    printf("char_count: %i\n", char_count);
    return char_count;
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


int get_lab_size(int window_index, int sb_size, int lab_size) {
    return (sb_size - window_index < lab_size) ? sb_size - window_index : lab_size;
}

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