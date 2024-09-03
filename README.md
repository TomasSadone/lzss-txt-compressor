# LZSS TXT COMPRESSOR

#### Video Demo: <URL HERE>

#### Description:

This repository contains a cli [lzss](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski) .txt files compressor, made with C.

Usage: ./main -i [input-file] -o [output-file] (optional) -m [**d** *for decompress* OR **c** *for compress*] (optional)

## Structs

I want to begin talking about the structs that are defined in this program, so we can better understand the rest.

### bit_buffer

```
typedef  struct bit_buffer {
	int head; //reference to the end of last whole byte written. (*buffer[head - 1] 		would access last byte)
	unsigned  char bit_count;
	unsigned  char bit_buffer;
	uint8_t* buffer;
} bit_buffer;
```

Exists because is not possible to write less than a byte to a file, and in this program we have the need to write as low as one single bit. With these struct and some helper functions (think of it as OOP) , we can write bit by bit untill we reach 8 (one byte), and then write it.

#### tuple

```
typedef  struct tuple {
	uint16_t offset;
	uint8_t length;
} tuple;
```

This is how the compressed data is stored, 2 bytes for the string match offset, and one byte for the string match length.

## Files

### main.c:

The main file of the program, it does many things: - Get arguments - Sanitze input - Handle file errors - Open files - Initialize the compress / decompress process.

#### Design choices

One design choice I debated here was whether to take arguments the standard way, and forcing the user to know the order of the commands, or to use the unistd library, which allows for flagged inputs. I went with the last one to allow the user for more flexibility. I also wanted to know how the flagged arguments worked in the CLI programs I use.
Another design choice I debated was to write the compressed output directly to a file, or to use the intermediate step of the buffer in memory as I ended up doing. It would've been easier writing directly to the file, but the way is done now allows to incorporate huffman encoding after lzss, transforming that way the program in a zip compressor

#### Functions:

Aside from the main function, whose responsibilities are mentioned under the "main.c" heading, it has the `compress` and `decompress` functions.
They both initialize buffers, check for errors, initialize bit buffers, and loop through the input buffer.

- compress: It has the particularity of writing the bits that were left in the bit buffer at the end of the compression.
- decompress: It's a necessity to uncompress the data as long as de uncompress data chunk size doesn't exceed 32kb, because if we continue uncompressing at that point, we might miss information that was placed in another buffer, to keep it simple.
  So to solve this problem, the function has to scroll back in the file n bytes left to decompress from this buffer.

### lzss.c

The file with the lzss and bit_buffer manipulation logic.

#### Design choices

The design choice I debated here was whether to keep the bit_buffer stuff here or not, as it can be viewed as a class that is big enough to have its own file, but I decided to keep it there because is closely coupled to the lzss functions in this file, this reality is easily changeable if needed in the future.

#### Functions:

`lzss_compress:`
This is a big one, but it determines the lookahead buffer size and the search buffer size based on the sliding window size, and it initializes them.
It then initializes the bit_buffer
After that, it goes to execute the main loop which has the logic for the string matching, which must retrieve the length and the offset (backwards from the current character) of the match.
Lastly, it shifts the buffers to look for the next character in the following iteration.
`lzss_decompress:`
The decompression is simpler.
It initializes the bit_buffer and keeps a char_count, needed for the breaking condition.
It then goes to the main loop, where it checks if the next information is a tuple to decompress, or a literal character, and writes it to the buffer.
It returns `char_count`, used to scroll backwards in the `decompress`
The functions related to the bit_buffer:
`bb_write_bit`
`bb_write_char`
`bb_write_tuple`
`bb_get_bit`
`bb_get_char`
`bb_get_tuple`
Are self explanatory by name.

## Debugging

I think talking about the debugging process allows for a better understanding of the program and why it is like it is, so I will explain some of the more time consuming bugs.
All the debugging of this project was hard to do because it consisted of looking through a lot of binary. Even if problems solved seem simple, they had that contingency

- I was writing the remaining when compressing even if there where no bits left in the bit_buffer, that was no problem when checking the out_buffer because the write_remaining function doesn't write if there's nothin to write, but the fwrite function obviously doesn't know which lead to writing the last byte two times in some occasions.

- I was skipping characters when compressing, but looking at the compressed binary it was just fine, and what was happening was moving the sliding window forward 2 bytes when the length of a match was 2, but I was only writing one byte to the out buffer.

- Then, when compressing, if offset was greater than 128, decompression would break from that point forward. The issue was that in the write_char function, I was recieving a char instead of a uint8_t or unsigned char, therefor I wasn't able to represent all the values I intended to.

- Lzss matches can end after the character in which the search started, this caused a problem for me when decompressing, because I was using the memcpy function to copy bytes from the decompressed buffer into the head of that buffer, but if the end of that memory copy I had to do was after the start of the memory destination, then the bytes weren't there to copy yet. My solution was implementing a for loop so the characters would have time to be written before being copied.

- Then I noticed that when decompressing, whenever I reached the 32510th uncompressed character (same size as search_buffer), I was skipping 258 characters (same as lookahead_buffer). This was due the breaking condition on the compress loop, that broke whenever the window_index was greaer thar 32510, but I still had those bytes left to compress, I had to rethink the indexing at that point.

- After that I had issues between one sliding_window and the other, at the 32th kb uncompressed. The issue was that I was losing the information remaining from the first window to the second one, I solved that by passing in the buffer externally to the lzss_compress and lzss_decompress functions, so I can keep track of the remaining bits and bit count.

Those were some of the major bugs I had to overcome when making this program, it was long, tedious, I had to come up with some functions to print binary values, but it was fun.

## What I learnt

I learnt a lot about binary, bitwise operations, debuggers use, the C type system, CLI tools usage, the C ecosystem (compilers, header files, commonly used resources). Also my speed of development in C has improved largely.

## What could be improved

- The compression algorithm can surely be improved, I had enough overhead figuring out how the program should work and didn't went deep into string matching algorithms.
- It could allow for more file types, compressing folders.
- It could integrate Huffman encoding, with some extra adjustments to transform itself inoto a zip compressor.
