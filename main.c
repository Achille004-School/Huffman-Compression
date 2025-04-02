#include "IO/bit_stream_reader.h"
#include "IO/bit_stream_writer.h"
#include "huffman/huffman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

int huffman_compress(BitSR reader, BitSW writer);
int huffman_decompress(BitSR reader, BitSW writer);

int main(int argc, char *argv[])
{
    // Check for correct number of arguments
    if (argc != 4)
    {
        printf("Usage: %s [zip/unzip] [input_file] [output_file]\n", argv[0]);
        return 1;
    }

    // Get command and filenames
    const char *command = argv[1];
    const char *input_file = argv[2];
    const char *output_file = argv[3];

    int outCode = EXIT_FAILURE;
    BitSR reader = NULL;
    BitSW writer = NULL;

    reader = bsr_create(input_file, 0);
    if (!reader)
    {
        perror("Error: Could not create reader\n");
        goto exit;
    }

    writer = bsw_create(output_file, 0);
    if (!writer)
    {
        perror("Error: Could not create writer\n");
        goto exit;
    }

    if (strcmp(command, "zip") == 0)
    {
        outCode = huffman_compress(reader, writer);
        if (outCode == EXIT_SUCCESS)
            printf("File compressed successfully.\n");
    }
    else if (strcmp(command, "unzip") == 0)
    {
        outCode = huffman_decompress(reader, writer);
        if (outCode == EXIT_SUCCESS)
            printf("File decompressed successfully.\n");
    }
    else
    {
        perror("Invalid command. Use 'zip' or 'unzip'.\n");
    }

exit:
    bsr_free(reader);
    bsw_free(writer);
    return outCode;
}

int huffman_compress(BitSR reader, BitSW writer)
{
    // Count frequencies of each byte in the input file
    uint8_t byte;
    int frequencies[POSSIBLE_BYTES] = {0};
    while (bsr_read_byte(reader, &byte))
        frequencies[byte]++;

    // Build Huffman tree and generate codes
    HuffmanTree tree = huffman_tree_create(frequencies);
    if (!tree)
    {
        perror("Error: Could not create Huffman tree\n");
        return EXIT_FAILURE;
    }
    
    huffman_tree_save(tree, writer);

    // Reset the reader to the beginning of the file
    bsr_rewind(reader);
    while (bsr_read_byte(reader, &byte))
    {
        // Write the Huffman code for the byte to the output file
        uint8_t *code = huffman_tree_get_code(tree, byte);
        int code_length = huffman_tree_get_code_length(tree, byte);

        if (code && code_length > 0)
        {
            if (code_length >= 8)
            {
                bsw_write_bytes(writer, code, code_length / 8);
                bsw_write_bits(writer, code + code_length / 8, code_length % 8);
            }
            else
            {
                bsw_write_bits(writer, code, code_length);
            }
        }
    }

    // Flush the writer buffer to ensure all data is written
    huffman_tree_destroy(tree);
    bsw_align_to_byte(writer);
    bsw_flush(writer);
    return EXIT_SUCCESS;
}

int huffman_decompress(BitSR reader, BitSW writer)
{
    // Build Huffman tree and generate codes
    HuffmanTree tree = huffman_tree_load(reader);
    if (!tree)
    {
        perror("Error: Could not create Huffman tree\n");
        return EXIT_FAILURE;
    }

    uint64_t bytesLeft = huffman_tree_total_frequencies(tree);
    uint8_t bit, byte;
    while (bsr_read_bit(reader, &bit) && bytesLeft > 0)
    {
        // Write the Huffman code for the byte to the output file
        int status = huffman_tree_decode_bit(tree, bit, &byte);

        switch (status)
        {
        case 0: // Continue reading bits
            break;
        case 1: // Found a byte, write it to the output file
            bsw_write_byte(writer, byte);
            bytesLeft--;
            break;
        case -1: // Error in decoding
            perror("Error: Could not decode Huffman code\n");
            huffman_tree_destroy(tree);
            return EXIT_FAILURE;
        }
    }

    // Flush the writer buffer to ensure all data is written
    huffman_tree_destroy(tree);
    bsw_align_to_byte(writer);
    bsw_flush(writer);
    return EXIT_SUCCESS;
}