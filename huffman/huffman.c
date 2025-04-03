#include "huffman.h"
#include "ptr_pq.h"
#include <stdlib.h>
#include <string.h>

// Huffman tree node structure
typedef struct HuffmanNode *link;
struct HuffmanNode
{
    uint8_t byte;
    int frequency;
    link left, right;
};

// Huffman tree structure
struct HuffmanTreeType
{
    link root;
    uint8_t **codes;   // Array to store codes for each byte (0-255)
    int *code_lengths; // Array to store code lengths

    link current; // Current node during decoding
};

static link huffman_node_create(uint8_t byte, int frequency);
static void huffman_node_free(link node);
static int huffman_node_compare(void *a, void *b);

static void generate_codes(HuffmanTree tree);
static void get_frequencies(HuffmanTree tree, int *frequencies);

HuffmanTree huffman_tree_create(int *frequencies)
{
    if (frequencies == NULL)
        return NULL;

    // Create a priority queue
    PtrPQ pq = ptr_pq_create(huffman_node_compare);
    if (pq == NULL)
        return NULL;

    // Add leaf nodes to the priority queue
    for (int i = 0; i < POSSIBLE_BYTES; i++)
        if (frequencies[i] > 0)
        {
            link node = huffman_node_create((uint8_t)i, frequencies[i]);
            if (node == NULL)
            {
                ptr_pq_free(pq);
                return NULL;
            }
            ptr_pq_insert(pq, node);
        }

    // Build the Huffman tree
    while (ptr_pq_size(pq) > 1)
    {
        int priority1, priority2;
        link left = (link)ptr_pq_extract_min(pq);
        link right = (link)ptr_pq_extract_min(pq);

        int sum_freq = left->frequency + right->frequency;
        link parent = huffman_node_create(0, sum_freq);
        if (parent == NULL)
        {
            huffman_node_free(left);
            huffman_node_free(right);
            ptr_pq_free(pq);
            return NULL;
        }

        parent->left = left;
        parent->right = right;
        ptr_pq_insert(pq, parent);
    }

    // Create the Huffman tree structure
    HuffmanTree tree = (HuffmanTree)malloc(sizeof *tree);
    if (tree == NULL)
    {
        ptr_pq_free(pq);
        return NULL;
    }

    // Get the root node (or NULL if the queue is empty)
    if (ptr_pq_size(pq) > 0)
    {
        int priority;
        tree->root = (link)ptr_pq_extract_min(pq);
    }
    else
    {
        tree->root = NULL;
    }

    // Initialize codes array
    tree->codes = (uint8_t **)calloc(POSSIBLE_BYTES, sizeof(uint8_t *));
    tree->code_lengths = (int *)calloc(POSSIBLE_BYTES, sizeof(int));
    if (tree->codes == NULL || tree->code_lengths == NULL)
    {
        huffman_tree_destroy(tree);
        ptr_pq_free(pq);
        return NULL;
    }

    // Generate codes
    if (tree->root != NULL)
        generate_codes(tree);

    tree->current = NULL; // Initialize current node for decoding
    ptr_pq_free(pq);
    return tree;
}

void huffman_tree_destroy(HuffmanTree tree)
{
    if (tree == NULL)
        return;

    // Free the codes
    if (tree->codes != NULL)
    {
        for (int i = 0; i < POSSIBLE_BYTES; i++)
            free(tree->codes[i]);
        free(tree->codes);
    }

    // Free the code lengths
    free(tree->code_lengths);

    // Free the tree nodes
    huffman_node_free(tree->root);

    // Free the tree structure
    free(tree);
}

HuffmanTree huffman_tree_load(BitSR reader)
{
    if (reader == NULL)
        return NULL;

    // Read frequencies from file
    uint8_t tempFreq[sizeof(int)];
    int frequencies[POSSIBLE_BYTES] = {0};
    int tmpFreq;
    while(true)
    {
        bsr_read_bytes(reader, tempFreq, sizeof(int));
        tmpFreq = (int)tempFreq[1] << 16 | (int)tempFreq[2] << 8 | (int)tempFreq[3];
        if (tmpFreq == 0)
            break; // End of frequencies
        frequencies[tempFreq[0]] = tmpFreq;
    }

    // Rebuild the Huffman tree using the frequencies
    HuffmanTree tree = huffman_tree_create(frequencies);
    return tree;
}

bool huffman_tree_save(HuffmanTree tree, BitSW writer)
{
    if (tree == NULL || writer == NULL)
        return 0;

    // Save frequencies (only used for tree reconstruction)
    int freqTemp[POSSIBLE_BYTES] = {0}; // Temporary buffer for writing frequencies
    get_frequencies(tree, freqTemp);

    uint8_t frequencies[sizeof(int)] = {0};
    for (int i = 0; i < POSSIBLE_BYTES; i++)
        if (freqTemp[i] > 0)
        {
            frequencies[0] = (uint8_t)i;          // Store the byte value in the first byte of the array
            for (int j = 1; j < sizeof(int); j++) // Store the frequency in the next 3 bytes
                frequencies[j] = (uint8_t)(freqTemp[i] >> (sizeof(uint8_t) * 8 * (sizeof(int) - 1 - j)));

            bsw_write_bytes(writer, frequencies, sizeof(int)); // Write the 4 bytes
        }

    for (int i = 0; i < sizeof(int); i++)
        bsw_write_byte(writer, 0);

    // Write frequencies to file
    bsw_align_to_byte(writer);
    return bsw_flush(writer);
}

uint8_t *huffman_tree_get_code(HuffmanTree tree, uint8_t byte)
{
    if (tree == NULL || tree->codes == NULL)
        return NULL;
    return tree->codes[byte];
}

int huffman_tree_get_code_length(HuffmanTree tree, uint8_t byte)
{
    if (tree == NULL || tree->code_lengths == NULL)
        return -1;
    return tree->code_lengths[byte];
}

uint64_t huffman_tree_total_frequencies(HuffmanTree tree)
{
    if (tree == NULL || tree->root == NULL)
        return 0;
    return tree->root->frequency;
}

int huffman_tree_decode_bit(HuffmanTree tree, int bit, uint8_t *decoded_byte)
{
    if (tree == NULL || decoded_byte == NULL)
        return -1;

    link current = tree->current;

    // Start from the root if current is NULL
    if (current == NULL)
    {
        current = tree->current = tree->root;
        if (current == NULL)
            return -1; // Tree is empty
    }

    // Traverse left or right based on the bit
    current = (tree->current = bit == 0 ? tree->current->left : tree->current->right);

    // Check if we reached a leaf node
    if (current != NULL && current->left == NULL && current->right == NULL)
    {
        *decoded_byte = current->byte;
        tree->current = NULL; // Reset for next decoding
        return 1;
    }

    return 0; // Need more bits
}

///// Helper functions /////

static link huffman_node_create(uint8_t byte, int frequency)
{
    link node = (link)malloc(sizeof *node);
    if (node == NULL)
    {
        return NULL;
    }

    node->byte = byte;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;

    return node;
}

static void huffman_node_free(link node)
{
    if (node == NULL)
        return;

    huffman_node_free(node->left);
    huffman_node_free(node->right);
    free(node);
}

static int huffman_node_compare(void *a, void *b)
{
    if (a == NULL && b == NULL)
        return 0;

    link aLink = (link)a, bLink = (link)b;
    return aLink->frequency - bLink->frequency;
}

static void generate_codes_rec(link node, uint8_t *code, int depth, uint8_t **codes, int *code_lengths);
static void generate_codes_rec(link node, uint8_t *code, int depth, uint8_t **codes, int *code_lengths)
{
    if (node == NULL)
        return;

    // If this is a leaf node, store its code
    if (node->left == NULL && node->right == NULL)
    {
        codes[node->byte] = (uint8_t *)malloc((depth + 7) / 8 + 1); // Allocate enough bytes to store all bits
        if (codes[node->byte] != NULL)
        {
            // Copy the bit pattern
            for (int i = 0; i < (depth + 7) / 8; i++)
            {
                codes[node->byte][i] = 0;
                for (int j = 0; j < 8 && (i * 8 + j < depth); j++)
                    if (code[i * 8 + j])
                        codes[node->byte][i] |= (1 << (7 - j)); // Set the bit if it's 1
            }
            codes[node->byte][(depth + 7) / 8] = '\0'; // Null terminator
            code_lengths[node->byte] = depth;
        }
        return;
    }

    // Traverse left with 0
    code[depth] = 0;
    generate_codes_rec(node->left, code, depth + 1, codes, code_lengths);

    // Traverse right with 1
    code[depth] = 1;
    generate_codes_rec(node->right, code, depth + 1, codes, code_lengths);
}

static void generate_codes(HuffmanTree tree)
{
    uint8_t code[POSSIBLE_BYTES] = {0}; // Temporary buffer for generating codes (oversized to 256 bits)
    generate_codes_rec(tree->root, code, 0, tree->codes, tree->code_lengths);
}

static void get_frequencies_rec(link node, int *frequencies);
static void get_frequencies_rec(link node, int *frequencies)
{
    if (node == NULL)
        return;

    // If this is a leaf node, store its frequency
    if (node->left == NULL && node->right == NULL)
    {
        frequencies[node->byte] = node->frequency;
        return;
    }

    get_frequencies_rec(node->left, frequencies);
    get_frequencies_rec(node->right, frequencies);
}

static void get_frequencies(HuffmanTree tree, int *frequencies)
{
    get_frequencies_rec(tree->root, frequencies);
}