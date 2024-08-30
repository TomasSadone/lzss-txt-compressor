#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "./lzss.h"

#define SLIDING_WINDOW_SIZE (1024 * 32) //~32.000kb
#define LOOKAHEAD_BUFFER_SIZE 258 //bytes
#define SEARCH_BUFFER_SIZE (SLIDING_WINDOW_SIZE - LOOKAHEAD_BUFFER_SIZE) 

void lzss_compress(uint8_t* sliding_window, int bf_size, uint8_t* out_buffer, bit_buffer *bb) {
    int lab_size = LOOKAHEAD_BUFFER_SIZE > bf_size ? bf_size : LOOKAHEAD_BUFFER_SIZE;
    int sb_size = SEARCH_BUFFER_SIZE > bf_size ? bf_size : SEARCH_BUFFER_SIZE;
    uint8_t lookahead_buffer[lab_size];
    memcpy(&lookahead_buffer[0], &sliding_window[0], lab_size);
    uint8_t search_buffer[sb_size];
    memset(search_buffer, 0, sb_size);
    int window_index = 0;
    int sb_index = 0;

    init_bit_buffer(out_buffer, bb);
    
    while (true) {
        int match_length = 0;
        int match_offset = 0;
        if (sb_index >= sb_size) {
            sb_index = sb_size - 1;
        }
        for (int i = sb_size - sb_index; i < sb_size; i++) {
            if(memcmp(&search_buffer[i], &lookahead_buffer[0], sizeof(uint8_t)) == 0) {
                int j = 1;
                int lhb_c_s = get_lab_size(window_index, bf_size, lab_size);
                while (i + j - sb_size < lhb_c_s && j < 255) {
                    uint8_t *current_buffer;
                    if (i + j < sb_size) {
                        current_buffer = &search_buffer[i + j];
                    } else {
                        current_buffer = &lookahead_buffer[i + j - sb_size];
                    }

                    if (memcmp(current_buffer, &lookahead_buffer[j], sizeof(uint8_t)) != 0) {
                        break;
                    }
                    ++j;
                }
                if (j > match_length) {
                    match_length = j;
                    match_offset = sb_size - i;
                }
            }
            if (match_length + i + 1 > sb_size) break;
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
        
        if(window_index + window_shift >= bf_size) {
            break;
        }
        //Shift search_buffer
            //shift entire search buffer window_shift times
            int dest_index = sb_size - sb_index - window_shift;
            int move_amount = sb_index;
            if (dest_index < 0) {
                dest_index = 0;
                move_amount = sb_size - window_shift;
            }

            int src_index = dest_index + window_shift;       
            if (src_index >= sb_size) {
                src_index = sb_size - 1;
            }

            memmove(&search_buffer[dest_index], &search_buffer[src_index], move_amount);

            //fill last garbage bytes with firsts from lookahead
            memcpy(&search_buffer[sb_size - window_shift], &lookahead_buffer[0], window_shift);

        //Shift lookahead_buffer
        window_index += window_shift;
        sb_index += window_shift;
        int next_lhbs =  get_lab_size(window_index, bf_size, lab_size);
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
            if (char_count + 1 > SLIDING_WINDOW_SIZE) {
                bb->bit_count++;   
                break;
            }
            out_buffer[char_count] = bb_get_byte(bb);
            char_count++;
        } else {
            tuple t = bb_get_tuple(bb);
            if ((char_count + t.length) > SLIDING_WINDOW_SIZE) {
                bb_write_tuple(bb, &t);
                bb_write_bit(bb, 1);
                break;
            }
            for (int i = 0; i < t.length; i++) {
                out_buffer[char_count + i] = out_buffer[char_count + i - t.offset ];
            }
            char_count += t.length;
        }
    }   
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


int get_lab_size(int window_index, int sw_size, int lab_size) {
    return (sw_size - window_index < lab_size) ? sw_size - window_index : lab_size;
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