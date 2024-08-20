#include <stdlib.h>
#include <stdbool.h>

#define SLIDING_WINDOW_SIZE (1024 * 32) //~32.000kb
#define LOOKAHEAD_BUFFER_SIZE 258 //bytes
#define SEARCH_BUFFER_SIZE (SLIDING_WINDOW_SIZE - LOOKAHEAD_BUFFER_SIZE) 

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

void lzss_compress(uint8_t* sliding_window, int bf_size, uint8_t* out_buffer) {
     
    int lab_size = LOOKAHEAD_BUFFER_SIZE > bf_size ? bf_size : LOOKAHEAD_BUFFER_SIZE;
    int sb_size = SEARCH_BUFFER_SIZE > bf_size ? bf_size : SEARCH_BUFFER_SIZE;
    uint8_t lookahead_buffer[lab_size];
    memcpy(&lookahead_buffer, sliding_window, lab_size);
    uint8_t search_buffer[sb_size];  
    memset(search_buffer, 0, sb_size);
    int window_index = 0;

    bit_buffer bb = init_bit_buffer(out_buffer);

    while (true) {
        int match_length = 0;
        int match_offset = 0;
        for (int i = sb_size - window_index; i < sb_size; i++) {
            if(memcmp(&search_buffer[i], &lookahead_buffer[0], sizeof(uint8_t)) == 0) {
                int current_match_length = 1;
                int j = 1;
                int lhb_c_s = get_lhb_size(window_index, sb_size, lab_size);
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
        int window_shift = match_length == 0 ? 1 : match_length;
        //Shift search_buffer
            //shift entire search buffer window_shift times
            memmove(&search_buffer[sb_size  - window_index - window_shift], &search_buffer[sb_size  - window_index], window_index);

            //fill last empty bytes with first
            memcpy(&search_buffer[sb_size - window_shift], &lookahead_buffer, window_shift);

        //si el match es > 2 pushear creo q 1 para indicar que es tuple, + tuple
        //si es <= 2, 0 + literal
        if (match_length > 2) {
            tuple node;
            node.offset = 500;
            node.length = match_length;
            bb_write_bit(&bb, 1);
            bb_write_tuple(&bb, &node);
        } else {
            bb_write_bit(&bb, 0);
            bb_write_char(&bb, lookahead_buffer[window_index]);
        }
        //Shift lookahead_buffer
        window_index += window_shift;    
        if(window_index >= bf_size) break;
        int next_lhbs =  get_lhb_size(window_index, sb_size, lab_size);
        memmove(&lookahead_buffer, &sliding_window[window_index], next_lhbs);
        memset(&lookahead_buffer[next_lhbs], 0, next_lhbs );
    }
 
    bb_write_remaining(&bb);
}

int get_lhb_size(int wi, int sbs, int lhbs) {
    return (sbs - wi < lhbs) ? sbs - wi : lhbs;
}

bit_buffer init_bit_buffer(uint8_t* buffer) {
    bit_buffer b;
    b.buffer = buffer;
    b.bit_count = 0;
    b.bit_buffer = 0;
    b.head = 0;
    return b;
};

void bb_write_bit(bit_buffer *bb, int bit) {
    bb->bit_count++;
    bb->bit_buffer <<= 1;
    bb->bit_buffer |= bit;
    if (bb->bit_count == 8) {
        bb->buffer[bb->head] = bb->bit_buffer;
        bb->head++;
    }
    bb->bit_buffer = 0;
    bb->bit_count = 0;
};

void bb_write_char(bit_buffer *bb, char c) {
    if (bb->bit_count == 0) {
        bb->buffer[bb->head] = c;
        bb->head++;
        return;
    }
    char tmp = c >> bb->bit_count;
    tmp |= (bb->bit_buffer << (8 - bb->bit_count));
    bb->bit_buffer = c;
    bb->buffer[bb->head] = tmp;
    bb->head++;
};


void bb_write_tuple(bit_buffer *bb, tuple *pt) {
    int i = 0;
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