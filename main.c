#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h> 
#include "./lzss.h"

#define SLIDING_WINDOW_SIZE (1024 * 32) //32kb

void print_binary(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (byte >> i) & 1);
    }
    printf(" ");
}

void print_binary_buffer(uint8_t *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        print_binary(buffer[i]);
    }
    printf("\n");
}

int compress(FILE *input_file, FILE *output_file);
int decompress(FILE *input_file, FILE *output_file);

int main(int argc,char* argv[]) {  
    int opt;
    char* input_file_name = NULL;
    char* output_file_name = NULL;
    char* mode = NULL;
    while ((opt = getopt(argc, argv, "i:o:m:")) != -1) {
        switch (opt) {
            case 'i':
                input_file_name = optarg;
                break;
            case 'o': 
                output_file_name = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            default:
                printf("Usage: %s [-i input_file] [-o output_file (optional)] [-m program_mode (optional)]\n", argv[0]);
                break;
        }
    }
    /*
        Eso es el uso minimo, crea una carpeta con ese nombre y ese archivo comprimido.
        Lugo mas adelante se puede agregar cosas como: Eliminar el archivo inicial, comprimir carpetas.
    */
    if (!input_file_name) {
        printf("Usage: %s [-i input_file] [-o output_file (optional)] [-m program_mode (optional)]\n", argv[0]);
        return 1;
    }
    if (!output_file_name) {
        output_file_name = "out.lzss.txt";
    }
    if (strcmp(input_file_name, output_file_name) == 0) {
        printf("Output and intput files must be diffrent\n");
        return 1;
    }
    if (!mode) {
        mode = "c";
    } else if (strcmp(mode, "c") != 0 && strcmp(mode, "d") != 0) {
        printf("Mode should be either 'c' for compress, 'd' for decompress, or not specified (compress by default)\n");
        return 1;
    }

   FILE* input_file = fopen(input_file_name, "r");
   FILE* output_file = fopen(output_file_name, "w");
    if (input_file == NULL) {
        printf("Error reading file\n");
        return 1;
    }
    if (output_file == NULL) {
        printf("Error creating output file\n");
        return 1;
    }
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
   int err = 0;
   switch (*mode) {
    case 'c':
        err = compress(input_file, output_file);
        break;
    case 'd':
        decompress(input_file, output_file);
    default:
        break;
   }
    if (err) {
        printf("Error compressing/decompressing the file\n");
    }
    fclose(input_file);
    fclose(output_file);
    return 0;
}

int compress(FILE *input_file, FILE *output_file) {
    uint8_t* in_buffer = calloc(SLIDING_WINDOW_SIZE, sizeof(uint8_t));
    uint8_t* out_buffer = calloc(SLIDING_WINDOW_SIZE, sizeof(uint8_t));
 
    if (in_buffer == NULL || out_buffer == NULL) {
        printf("Error allocating memory for in_buffer");
        free(in_buffer);
        free(out_buffer);
        fclose(input_file);
        return 1;
    }

    int bf_size = fread(in_buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);
    bit_buffer bb;
    bb.bit_count = 0;

    while ((bf_size > 0)) {
        lzss_compress(in_buffer, bf_size, out_buffer, &bb);
        //huffman_compress(in_buffer);
        fwrite(bb.buffer, sizeof(uint8_t), bb.head, output_file);
        bf_size = fread(in_buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);
    }

    //creo q temporal, mientras no exista huffman
        bb_write_remaining(&bb);
        fwrite(&bb.buffer[bb.head - 1], sizeof(uint8_t), 1, output_file);
    free(in_buffer);
    free(out_buffer);    
    return 0;
}

int decompress(FILE *input_file, FILE *output_file) {
    uint8_t* in_buffer = calloc(SLIDING_WINDOW_SIZE, sizeof(uint8_t));
    uint8_t* out_buffer = calloc(SLIDING_WINDOW_SIZE, sizeof(uint8_t));
 
    if (in_buffer == NULL || out_buffer == NULL) {
        printf("Error allocating memory for in_buffer");
        free(in_buffer);
        free(out_buffer);
        fclose(input_file);
        return 1;
    }

    int bf_size = fread(in_buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);
    bit_buffer bb;
    bb.bit_count = 0;
    while ((bf_size > 0)) {
        // huffman_decompress(in_buffer);
        int out_b_size = lzss_decompress(in_buffer, bf_size, out_buffer, &bb);
        fwrite(out_buffer, sizeof(uint8_t), out_b_size, output_file);
        fseek(input_file, out_b_size - bf_size, SEEK_CUR);
        // print_binary_buffer(in_buffer, bf_size);
        bf_size = fread(in_buffer, sizeof(u_int8_t), SLIDING_WINDOW_SIZE, input_file);

    }
    
    free(in_buffer);
    free(out_buffer);    
    return 0;
}