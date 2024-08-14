#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define SLIDING_WINDOW_SIZE (1024 * 32) //~32.000kb
#define LOOKAHEAD_BUFFER_SIZE 258 //bytes
#define SEARCH_BUFFER_SIZE (SLIDING_WINDOW_SIZE - LOOKAHEAD_BUFFER_SIZE) 

typedef struct tuple {
    uint16_t offset; //representa un valor de 1 - 32,768 bytes;
    uint8_t length; // representa un valor de 3 - 258 bytes;
    // https://github.com/MichaelDipperstein/lzss/blob/master/lzss.c linea 162 tener en cuenta para el length de 3 - 258
    // .. lo dice tambien en moddingwiki decoding procedure 
} tuple;

typedef struct bit_buffer {
    int head; //reference to the end of last whole byte written. (*buffer[head - 1] would access last byte)
    unsigned char bit_count;
    unsigned char bit_buffer;
    uint8_t* buffer;
} bit_buffer;

void lzss_compress(uint8_t* sliding_window, int bf_size, uint8_t* out_buffer);
int get_lhb_size(int wi, int sbs, int lhbs);
bit_buffer init_bit_buffer(uint8_t* buffer);
void bb_write_bit(bit_buffer *bb, int bit);
void bb_write_char(bit_buffer *bb, char c);
void bb_write_tuple(bit_buffer *bb, tuple *pt);
void bb_write_tuple(bit_buffer *bb, tuple *pt) ;
void bb_write_remaining(bit_buffer *bb);

//00000001 1 en binario
//00000000 0 en binario

int main(int argc,char* argv[]) {    if (argc != 2) {
        printf("Usage: bytes4sale './filepath' (in) './filepath' (out, optional) \n"); //TODO adaptar al correcto uso
        return 1;
    }

    /*
        bytes4sale ../archivo-a-comprimir
        Eso es el uso minimo, crea una carpeta con ese nombre y ese archivo comprimido.
        Lugo mas adelante se puede agregar cosas como: Eliminar el archivo inicial, comprimir carpetas,
        nombrar el archivo de destino.

        // Leer el archivo
            - Si no es valido, error [X]
            - Si args no son los correctos, error. [X]
            - Comprimir de a 32kb, primero lzss, despues huffman
    */
   char* input_file_name = argv[1];
   char* output_file_name = argv[2] ? argv[2] : "out.lzss.txt";

   FILE* input_file = fopen(input_file_name, "r");
   FILE* output_file = fopen(output_file_name, "w");

   if (input_file == NULL) {
        printf("Error reading file\n");
        return 1;
   }

    //strchr se puede usar para buscar en lzss; o no.
    char* ext = strrchr(input_file_name, '.');

    if (strcmp(".txt", ext) != 0) {
        printf("Only text files supported currently");
        return 1;
    }
    //TODO: soportar mas archivos, definiendo los headers en otro archivo y leyendolos aca haciendo un switch para definir el sizeof
    /******
    pseudo:
    int size;
    switch(ext)
        case 'jpg': 
        case 'jpeg': 
            size = sizeof(jpgheaders)
    while(fread(header, size, )) etc eyc. {escribirlos donde se deba}
    -- creo que no va el while si no que solo con leerlo al buffer alcanza.
    ******/

    uint8_t* in_buffer = calloc(SLIDING_WINDOW_SIZE, sizeof(uint8_t));
    uint8_t* out_buffer = calloc(SLIDING_WINDOW_SIZE, sizeof(uint8_t));
 
    if (in_buffer == NULL) {
        printf("Error allocating memory for in_buffer");
        free(in_buffer);
        fclose(input_file);
        return 1;
    }
    int bf_size = fread(in_buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);

    //TODO crear archivo zip donde despues vamos a ir escribiendo 
    while ((bf_size > 0)) {
        lzss_compress(in_buffer, bf_size, out_buffer);
        //huffman_compress(in_buffer);
        //fwrite(output_name, in_buffer); algo asi no se como se usa fwrite.
        bf_size = fread(in_buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);
    }
    free(in_buffer);
    free(out_buffer);
    fclose(input_file);
    fclose(output_file);
    return 0;
}

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