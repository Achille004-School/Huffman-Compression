#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdint.h>
#include "../IO/bit_stream_reader.h"
#include "../IO/bit_stream_writer.h"

#define POSSIBLE_BYTES 256 // Number of possible byte values (0-255)

typedef struct HuffmanTreeType* HuffmanTree;

// Function to create a Huffman tree from frequency data
HuffmanTree huffman_tree_create(int *frequencies);

// Function to destroy the Huffman tree
void huffman_tree_destroy(HuffmanTree tree);

// Function to load the Huffman tree from a BisSR
HuffmanTree huffman_tree_load(BitSR reader);

// Function to save the Huffman tree to a BitSW
bool huffman_tree_save(HuffmanTree tree, BitSW writer);

// Function to get the code for a specific byte
uint8_t *huffman_get_code(HuffmanTree tree, uint8_t byte);

// Function to get the code length for a specific byte
int huffman_get_code_length(HuffmanTree tree, uint8_t byte);

// Function to decode a single bit
// Returns 1 if a byte was decoded, 0 if more bits are needed, -1 on error
int huffman_decode_bit(HuffmanTree tree, int bit, uint8_t *decoded_byte);

#endif /* HUFFMAN_H */