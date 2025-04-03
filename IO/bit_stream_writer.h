#ifndef BIT_STREAM_WRITER_H
#define BIT_STREAM_WRITER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief BitStreamWriter - A buffered bit stream writer ADT
 *
 * This ADT provides functionality to write individual bits or groups of bits
 * to a file with buffering for efficiency.
 */
typedef struct BitStreamWriter *BitSW;

/**
 * @brief Creates a new BitStreamWriter for the given file
 *
 * @param filename The path to the file to write
 * @param buffer_size Size of the internal buffer in bytes
 * @return A new BitStreamWriter instance or NULL if creation failed
 */
BitSW bsw_create(const char *filename, size_t buffer_size);

/**
 * @brief Creates a new BitStreamWriter for an already open file
 *
 * @param file An open file handle with write permissions
 * @param buffer_size Size of the internal buffer in bytes
 * @param close_on_free Whether to close the file when the writer is freed
 * @return A new BitStreamWriter instance or NULL if creation failed
 */
BitSW bsw_create_from_file(const char *filename, size_t buffer_size, bool close_on_free);

/**
 * @brief Frees all resources associated with the BitStreamWriter
 *
 * Flushes any pending bits to the file and closes the file if it was
 * opened by the BitStreamWriter or if close_on_free was set to true.
 *
 * @param writer The BitStreamWriter to free
 */
void bsw_free(BitSW writer);

/**
 * @brief Writes a single bit to the stream
 *
 * @param writer The BitStreamWriter
 * @param bit The bit to write (0 or 1)
 * @return true if successful, false on error
 */
bool bsw_write_bit(BitSW writer, uint8_t bit);

/**
 * @brief Writes multiple bits from an unsigned long long value
 *
 * Writes the specified number of least significant bits from the provided value.
 *
 * @param writer The BitStreamWriter
 * @param value The bits to write
 * @param num_bits The number of bits to write (must be <= 64)
 * @return Number of bits successfully written
 */
size_t bsw_write_bits(BitSW writer, uint8_t *value, size_t num_bits);

/**
 * @brief Writes a byte (8 bits) to the stream
 *
 * @param writer The BitStreamWriter
 * @param byte The byte to write
 * @return true if successful, false on error
 */
bool bsw_write_byte(BitSW writer, uint8_t byte);

/**
 * @brief Writes multiple bytes to the stream
 *
 * @param writer The BitStreamWriter
 * @param data The bytes to write
 * @param size The number of bytes to write
 * @return Number of bytes successfully written
 */
size_t bsw_write_bytes(BitSW writer, uint8_t *data, size_t size);

/**
 * @brief Flushes any pending bits in the buffer to the file
 *
 * Incomplete bytes are padded with zeros.
 *
 * @param writer The BitStreamWriter
 * @return true if successful, false on error
 */
bool bsw_flush(BitSW writer);

/**
 * @brief Aligns the bit position to the next byte boundary
 *
 * Pads any remaining bits in the current byte with zeros.
 *
 * @param writer The BitStreamWriter
 * @return true if successful, false on error
 */
bool bsw_align_to_byte(BitSW writer);

/**
 * @brief Returns the total number of bits written so far
 *
 * @param writer The BitStreamWriter
 * @return The number of bits written
 */
uint64_t bsw_get_bits_written(BitSW writer);

/**
 * @brief Returns the total number of bytes written to the file
 *
 * This counts only completely written bytes (including those still in the buffer).
 *
 * @param writer The BitStreamWriter
 * @return The number of bytes written
 */
uint64_t bsw_get_bytes_written(BitSW writer);

/**
 * @brief Returns the file pointer associated with the BitStreamReader
 *
 * @param reader The BitStreamReader
 * @return The file pointer
 */
FILE *bsw_get_file(BitSW writer);

/**
 * @brief Checks if an error occurred during any operation
 *
 * @param writer The BitStreamWriter
 * @return true if an error occurred, false otherwise
 */
bool bsw_has_error(BitSW writer);

/**
 * @brief Resets the error state
 *
 * @param writer The BitStreamWriter
 */
void bsw_clear_error(BitSW writer);

#endif /* BIT_STREAM_WRITER_H */