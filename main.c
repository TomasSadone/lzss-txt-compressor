#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


#define SLIDING_WINDOW_SIZE (1024 * 32) //~32.000kb
#define LOOKAHEAD_BUFFER_SIZE 258 //bytes
#define SEARCH_BUFFER_SIZE (SLIDING_WINDOW_SIZE - LOOKAHEAD_BUFFER_SIZE) 

typedef struct tuple {
    int offset : 15; //representa un valor de 1 - 32,768 bytes;
    int length : 8; // representa un valor de 3 - 258 bytes;
} tuple;

void lzss_compress(uint8_t *sliding_window, int bf_size);
int get_lhb_size(int wi, int sbs, int lhbs);


int main(int argc,char* argv[]) {
    if (argc != 2) {
        printf("Usage: bytes4sale './filepath'\n"); //TODO adaptar al correcto uso
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

   FILE* input_file = fopen(input_file_name, "r");

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
    ******/

    uint8_t* buffer = calloc(SLIDING_WINDOW_SIZE, sizeof(uint8_t));

    if (buffer == NULL) {
        printf("Error allocating memory for buffer");
        free(buffer);
        fclose(input_file);
        return 1;
    }
    int bf_size = fread(buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);
    //TODO crear archivo zip donde despues vamos a ir escribiendo 
    while ((bf_size > 0)) {
        lzss_compress(buffer, bf_size);
        //huffman_compress(buffer);
        //fwrite(output_name, buffer); algo asi no se como se usa fwrite.
        bf_size = fread(buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);
    }
    free(buffer);
    fclose(input_file);
    return 0;
}

void lzss_compress(uint8_t* sliding_window, int bf_size) {
     
    int lab_size = LOOKAHEAD_BUFFER_SIZE > bf_size ? bf_size : LOOKAHEAD_BUFFER_SIZE;
    int sb_size = SEARCH_BUFFER_SIZE > bf_size ? bf_size : SEARCH_BUFFER_SIZE;
    uint8_t lookahead_buffer[lab_size];
    memcpy(&lookahead_buffer, sliding_window, lab_size);
    uint8_t search_buffer[sb_size];  
    memset(search_buffer, 0, sb_size);
    int window_index = 0;

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
            //shift entire search buffer match_length times
            memmove(&search_buffer[sb_size  - window_index - window_shift], &search_buffer[sb_size  - window_index], window_index);

            //fill last empty bytes with first
            memcpy(&search_buffer[sb_size - window_shift], &lookahead_buffer, window_shift);

        //Shift lookahead_buffer
        window_index += window_shift;    
        if(window_index >= bf_size) break;
        int next_lhbs =  get_lhb_size(window_index, sb_size, lab_size);
        memmove(&lookahead_buffer, &sliding_window[window_index], next_lhbs);
        memset(&lookahead_buffer[next_lhbs], 0, next_lhbs );
        //si el match es > 2 pushear creo q 1 para indicar que es tuple, + tuple
        //si es <= 2, 0 + literal
    }
 
}

int get_lhb_size(int wi, int sbs, int lhbs) {
    return (sbs - wi < lhbs) ? sbs - wi : lhbs;
}
