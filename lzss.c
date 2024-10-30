#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "./lzss.h"

#define SLIDING_WINDOW_SIZE (1024 * 32) //~32kb
#define LOOKAHEAD_BUFFER_SIZE 258       // bytes
#define SEARCH_BUFFER_SIZE (SLIDING_WINDOW_SIZE - LOOKAHEAD_BUFFER_SIZE)

// typedef struct node
// {
//     int index;
//     node *next
// } node;

void lzss_compress(uint8_t *sliding_window, int bf_size, uint8_t *out_buffer, bit_buffer *bb)
{
    int lab_size = LOOKAHEAD_BUFFER_SIZE > bf_size ? bf_size : LOOKAHEAD_BUFFER_SIZE;
    int sb_size = SEARCH_BUFFER_SIZE > bf_size ? bf_size : SEARCH_BUFFER_SIZE;
    uint8_t lookahead_buffer[lab_size];
    memcpy(&lookahead_buffer[0], &sliding_window[0], lab_size);
    uint8_t search_buffer[sb_size];
    memset(search_buffer, 0, sb_size);
    int window_index = 0;
    int sb_index = 0;

    init_bit_buffer(out_buffer, bb);

    while (true)
    {
        int lhb_c_s = get_lab_size(window_index, bf_size, lab_size);
        match m = get_match(sb_size, sb_index, search_buffer, lookahead_buffer, lhb_c_s);

        if (sb_index >= sb_size)
        {
            sb_index = sb_size - 1;
        }

        // if there's no match, move window one position anyways.
        int window_shift = m.length <= 2 ? 1 : m.length;

        handle_match(bb, &m, lookahead_buffer[0]);

        if (window_index + window_shift >= bf_size)
        {
            break;
        }

        // Shift search_buffer
        // shift entire search buffer window_shift times
        shift_search_buffer(sb_size, sb_index, window_shift, search_buffer, lookahead_buffer);

        // Shift lookahead_buffer
        window_index += window_shift;
        sb_index += window_shift;
        shift_lookahead_buffer(window_index, bf_size, lab_size, lookahead_buffer, sliding_window);
    }
}

int lzss_decompress(uint8_t *in_buffer, int bf_size, uint8_t *out_buffer, bit_buffer *bb)
{
    init_bit_buffer(in_buffer, bb);
    int char_count = 0;
    while (bb->head < bf_size)
    {
        if (bb_get_bit(bb) == 0)
        {
            if (char_count + 1 > SLIDING_WINDOW_SIZE)
            {
                bb->bit_count++;
                break;
            }
            out_buffer[char_count] = bb_get_byte(bb);
            char_count++;
        }
        else
        {
            tuple t = bb_get_tuple(bb);
            if ((char_count + t.length) > SLIDING_WINDOW_SIZE)
            {
                bb_write_tuple(bb, &t);
                bb_write_bit(bb, 1);
                break;
            }
            for (int i = 0; i < t.length; i++)
            {
                out_buffer[char_count + i] = out_buffer[char_count + i - t.offset];
            }
            char_count += t.length;
        }
    }
    return char_count;
}

int get_lab_size(int window_index, int sw_size, int lab_size)
{
    return (sw_size - window_index < lab_size) ? sw_size - window_index : lab_size;
}

match get_match(int sb_size, int sb_index, uint8_t *search_buffer, uint8_t *lookahead_buffer, int lhb_c_s)
{
    // al cambiar a hash table, valores desactualizados chequearlos al momento de accederlos para no loopear solo para eso
    match m;
    m.length = 0;
    m.offset = 0;
    for (int i = sb_size - sb_index; i < sb_size; i++)
    {
        if (memcmp(&search_buffer[i], &lookahead_buffer[0], sizeof(uint8_t)) == 0)
        {
            int j = 1;
            while (i + j - sb_size < lhb_c_s && j < 255)
            {
                uint8_t *current_buffer;
                if (i + j < sb_size)
                {
                    current_buffer = &search_buffer[i + j];
                }
                else
                {
                    current_buffer = &lookahead_buffer[i + j - sb_size];
                }

                if (memcmp(current_buffer, &lookahead_buffer[j], sizeof(uint8_t)) != 0)
                {
                    break;
                }
                ++j;
            }
            if (j > m.length)
            {
                m.length = j;
                m.offset = sb_size - i;
            }
        }
        if (m.length + i + 1 > sb_size)
            break;
    }
    return m;
}

void handle_match(bit_buffer *bb, match *m, char c)
{
    if (m->length > 2)
    {
        tuple node;
        node.offset = m->offset;
        node.length = m->length;
        bb_write_bit(bb, 1);
        bb_write_tuple(bb, &node);
    }
    else
    {
        bb_write_bit(bb, 0);
        char written_char = bb_write_char(bb, c);
    }
}

void shift_search_buffer(int sb_size, int sb_index, int window_shift, uint8_t *search_buffer, uint8_t *lookahead_buffer)
{
    int dest_index = sb_size - sb_index - window_shift;
    int move_amount = sb_index;
    if (dest_index < 0)
    {
        dest_index = 0;
        move_amount = sb_size - window_shift;
    }

    int src_index = dest_index + window_shift;
    if (src_index >= sb_size)
    {
        src_index = sb_size - 1;
    }

    memmove(&search_buffer[dest_index], &search_buffer[src_index], move_amount);

    // fill last garbage bytes with firsts from lookahead
    memcpy(&search_buffer[sb_size - window_shift], &lookahead_buffer[0], window_shift);
}

void shift_lookahead_buffer(int window_index, int bf_size, int lab_size, uint8_t *lookahead_buffer, uint8_t *sliding_window)
{
    int next_lhbs = get_lab_size(window_index, bf_size, lab_size);
    memmove(lookahead_buffer, &sliding_window[window_index], next_lhbs);
    if (next_lhbs < lab_size)
    {
        memset(&lookahead_buffer[next_lhbs], 0, lab_size - next_lhbs);
    }
}