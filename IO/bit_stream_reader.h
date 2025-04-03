#ifndef BIT_STREAM_READER_H
#define BIT_STREAM_READER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief BitStreamReader - A buffered bit stream reader ADT
 * 
 * This ADT provides functionality to read individual bits or groups of bits
 * from a file with buffering for efficiency.
 */
typedef struct BitStreamReader *BitSR;

/**
 * @brief Creates a new BitStreamReader for the given file
 * 
 * @param filename The path to the file to read
 * @param buffer_size Size of the internal buffer in bytes
 * @return A new BitStreamReader instance or NULL if creation failed
 */
BitSR bsr_create(const char* filename, size_t buffer_size);

/**
 * @brief Creates a new BitStreamReader for an already open file
 * 
 * @param file An open file handle with read permissions
 * @param buffer_size Size of the internal buffer in bytes
 * @param close_on_free Whether to close the file when the reader is freed
 * @return A new BitStreamReader instance or NULL if creation failed
 */
BitSR bsr_create_from_file(const char* filename, size_t buffer_size, bool close_on_free);

/**
 * @brief Frees all resources associated with the BitStreamReader
 * 
 * Closes the file if it was opened by the BitStreamReader or if close_on_free was set to true.
 * 
 * @param reader The BitStreamReader to free
 */
void bsr_free(BitSR reader);

/**
 * @brief Reads a single bit from the stream
 * 
 * @param reader The BitStreamReader
 * @param bit Pointer to store the read bit (0 or 1)
 * @return true if successful, false on error or end of file
 */
bool bsr_read_bit(BitSR reader, uint8_t* bit);

/**
 * @brief Reads multiple bits into an unsigned long long value
 * 
 * Reads the specified number of bits and stores them in the provided value.
 * 
 * @param reader The BitStreamReader
 * @param value Pointer to store the read bits
 * @param num_bits The number of bits to read
 * @return Number of bits successfully read, may be less than size at EOF
 */
size_t bsr_read_bits(BitSR reader, uint8_t* value, size_t num_bits);

/**
 * @brief Reads a byte (8 bits) from the stream
 * 
 * @param reader The BitStreamReader
 * @param byte Pointer to store the read byte
 * @return true if successful, false on error or end of file
 */
bool bsr_read_byte(BitSR reader, uint8_t* byte);

/**
 * @brief Reads multiple bytes from the stream
 * 
 * @param reader The BitStreamReader
 * @param data Buffer to store the read bytes
 * @param size The number of bytes to read
 * @return Number of bytes successfully read, may be less than size at EOF
 */
size_t bsr_read_bytes(BitSR reader, uint8_t* data, size_t size);

/**
 * @brief Aligns the bit position to the next byte boundary
 * 
 * Discards any remaining bits in the current byte.
 * 
 * @param reader The BitStreamReader
 * @return true if successful, false on error
 */
bool bsr_align_to_byte(BitSR reader);

/**
 * @brief Returns the total number of bits read so far
 * 
 * @param reader The BitStreamReader
 * @return The number of bits read
 */
uint64_t bsr_get_bits_read(BitSR reader);

/**
 * @brief Returns the total number of bytes read from the file
 * 
 * @param reader The BitStreamReader
 * @return The number of bytes read
 */
uint64_t bsr_get_bytes_read(BitSR reader);

/**
 * @brief Rewinds the BitStreamReader to the beginning of the file
 * 
 * Resets the internal buffer and bit position.
 * 
 * @param reader The BitStreamReader
 */
void bsr_rewind(BitSR reader);

/**
 * @brief Checks if end of file has been reached
 * 
 * @param reader The BitStreamReader
 * @return true if end of file reached, false otherwise
 */
bool bsr_is_eof(BitSR reader);

/**
 * @brief Checks if an error occurred during any operation
 * 
 * @param reader The BitStreamReader
 * @return true if an error occurred, false otherwise
 */
bool bsr_has_error(BitSR reader);

/**
 * @brief Resets the error state
 * 
 * @param reader The BitStreamReader
 */
void bsr_clear_error(BitSR reader);

#endif /* BIT_STREAM_READER_H */