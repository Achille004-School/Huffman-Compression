#include "bit_stream_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUFFER_SIZE 1024

struct BitStreamReader
{
    FILE *file;           // Input file
    uint8_t *buffer;      // Buffer for reading bytes from file
    size_t buffer_size;   // Size of the buffer in bytes
    size_t buffer_pos;    // Current position in the buffer
    size_t buffer_filled; // Number of bytes in buffer currently filled
    uint8_t current_byte; // Current byte being read from
    uint8_t bit_pos;      // Position within the current byte (0-7)
    uint64_t total_bits;  // Total number of bits read
    bool has_error;       // Error flag
    bool is_eof;          // End of file flag
    bool close_on_free;   // Whether to close the file when freeing
};

// Forward declarations of static functions
static bool refill_buffer(BitSR reader);
static bool read_next_byte(BitSR reader);

BitSR bsr_create(const char *filename, size_t buffer_size)
{
    if (!filename)
        return NULL;

    BitSR reader = bsr_create_from_file(filename, buffer_size, true);
    return reader;
}

BitSR bsr_create_from_file(const char *filename, size_t buffer_size, bool close_on_free)
{
    if (!filename)
        return NULL;

    FILE *file = fopen(filename, "rb");
    if (!file)
        return NULL;

    BitSR reader = (BitSR)malloc(sizeof *reader);
    if (!reader)
    {
        fclose(file);
        return NULL;
    }

    if (buffer_size == 0)
        buffer_size = DEFAULT_BUFFER_SIZE;

    reader->buffer = (uint8_t *)malloc(buffer_size);
    if (!reader->buffer)
    {
        free(reader);
        return NULL;
    }

    reader->file = file;
    reader->buffer_size = buffer_size;
    reader->buffer_pos = 0;
    reader->buffer_filled = 0;
    reader->current_byte = 0;
    reader->bit_pos = 8; // Force reading a new byte on first read
    reader->total_bits = 0;
    reader->has_error = false;
    reader->is_eof = false;
    reader->close_on_free = close_on_free;

    return reader;
}

void bsr_free(BitSR reader)
{
    if (!reader)
        return;

    // Close file if needed
    if (reader->file && reader->close_on_free)
        fclose(reader->file);

    // Free resources
    free(reader->buffer);
    free(reader);
}

bool bsr_read_bit(BitSR reader, uint8_t *bit)
{
    if (!reader || !bit || reader->has_error)
        return false;

    // If we've read all bits in current byte, get next byte
    if (!read_next_byte(reader))
        return false; // EOF or error occurred

    // Extract the bit at the current position
    *bit = (reader->current_byte >> (7 - reader->bit_pos)) & 1;

    // Increment position
    reader->bit_pos++;
    reader->total_bits++;

    return true;
}

size_t bsr_read_bits(BitSR reader, uint8_t *value, size_t num_bits)
{
    if (!reader || !value || reader->has_error)
        return 0;

    // Read bits one at a time from most significant to least significant
    uint8_t bit;
    for (size_t i = 0; i < num_bits; i++)
        if (!bsr_read_bit(reader, value + i))
            return i;

    return num_bits;
}

bool bsr_read_byte(BitSR reader, uint8_t *byte)
{
    if (!reader || !byte || reader->has_error)
        return false;

    // If we've read all bits in current byte, get next byte
    if (!read_next_byte(reader))
        return false; // EOF or error occurred

    // If we're at a byte boundary, read directly for better performance
    if (reader->bit_pos == 0)
    {
        *byte = reader->current_byte;
        reader->bit_pos = 8; // Mark that we've read the whole byte
        reader->total_bits += 8;
        return true;
    }

    // Otherwise, read bit by bit
    uint8_t value[8] = {0}; // Temporary array to hold bits
    size_t read = bsr_read_bits(reader, value, 8);

    if (read == 8)
    {
        *byte = 0; // Initialize byte to 0
        for (int i = 0; i < 8; i++)
            *byte |= (value[i] << (7 - i)); // Combine bits into byte

        reader->total_bits += 8;
        return true;
    }
    return false;
}

size_t bsr_read_bytes(BitSR reader, uint8_t *data, size_t size)
{
    if (!reader || !data || reader->has_error)
        return 0;

    size_t bytes_read = 0;

    // If we've read all bits in current byte, get next byte
    if (!read_next_byte(reader))
        return bytes_read; // EOF or error occurred

    // If we're at a byte boundary and have a lot of data, use optimized path
    if (reader->bit_pos == 0)
    {
        // If we have a partially read byte, read the first byte normally
        if (!bsr_read_byte(reader, &data[0]))
            return bytes_read;
        bytes_read = 1;

        // If that was all we needed, return now
        if (bytes_read == size)
            return bytes_read;

        // Read any leftover partial data in the buffer
        if (reader->buffer_pos < reader->buffer_filled)
        {
            size_t buffer_remaining = reader->buffer_filled - reader->buffer_pos;
            size_t to_copy = (size - bytes_read) < buffer_remaining ? (size - bytes_read) : buffer_remaining;

            memcpy(data + bytes_read, reader->buffer + reader->buffer_pos, to_copy);
            reader->buffer_pos += to_copy;
            reader->total_bits += to_copy * 8;
            bytes_read += to_copy;

            // If we've read all requested bytes, return
            if (bytes_read == size)
                return bytes_read;
        }

        // Reset buffer for direct reading
        reader->buffer_pos = reader->buffer_filled = 0;

        // Read large chunks directly into the destination buffer
        if ((size - bytes_read) >= reader->buffer_size)
        {
            size_t direct_bytes = fread(data + bytes_read, 1, size - bytes_read, reader->file);
            if (direct_bytes < (size - bytes_read) && ferror(reader->file))
                reader->has_error = true;
            if (direct_bytes == 0 && feof(reader->file))
                reader->is_eof = true;

            reader->total_bits += direct_bytes * 8;
            bytes_read += direct_bytes;
            return bytes_read;
        }

        // Refill buffer and continue with byte-by-byte reading
        if (!refill_buffer(reader) && reader->buffer_filled == 0)
            return bytes_read;
    }

    // For any remaining bytes or if we're not byte-aligned, read byte-by-byte
    while (bytes_read < size)
    {
        if (!bsr_read_byte(reader, data + bytes_read))
            break;
        bytes_read++;
    }

    return bytes_read;
}

bool bsr_align_to_byte(BitSR reader)
{
    if (!reader || reader->has_error)
        return false;

    // If already aligned, nothing to do
    if (reader->bit_pos == 0 || reader->bit_pos == 8)
        return true;

    // Skip remaining bits in current byte
    reader->bit_pos = 8;
    return true;
}

uint64_t bsr_get_bits_read(BitSR reader)
{
    if (!reader)
        return 0;

    return reader->total_bits;
}

uint64_t bsr_get_bytes_read(BitSR reader)
{
    if (!reader)
        return 0;

    // Count full bytes read including those from the buffer
    return reader->total_bits / 8;
}

void bsr_rewind(BitSR reader)
{
    if (!reader || reader->has_error)
        return;

    // Reset buffer and file position
    reader->buffer_pos = 0;
    reader->buffer_filled = 0;
    reader->total_bits = 0;
    reader->bit_pos = 8; // Force reading a new byte on next read

    // Rewind the file to the beginning
    if (reader->file)
    {
        fseek(reader->file, 0, SEEK_SET);
        reader->is_eof = feof(reader->file);
        reader->has_error = ferror(reader->file);
    }
}

bool bsr_is_eof(BitSR reader)
{
    if (!reader)
        return true;

    return reader->is_eof;
}

bool bsr_has_error(BitSR reader)
{
    if (!reader)
        return true;

    return reader->has_error;
}

void bsr_clear_error(BitSR reader)
{
    if (reader)
        reader->has_error = false;
}

///// Static Helper Functions /////

static bool refill_buffer(BitSR reader)
{
    if (reader->is_eof)
        return false;

    reader->buffer_filled = fread(reader->buffer, 1, reader->buffer_size, reader->file);
    reader->buffer_pos = 0;

    if (reader->buffer_filled == 0)
    {
        if (feof(reader->file))
            reader->is_eof = true;
        else
            reader->has_error = true;
        return false;
    }
    return true;
}

static bool read_next_byte(BitSR reader)
{
    // If we've not read all bits in current byte, ignore the call
    if (reader->bit_pos != 8)
        return true;

    // If we're at the end of the buffer (or haven't read yet), refill
    if (reader->buffer_pos >= reader->buffer_filled)
        if (!refill_buffer(reader))
            return false;

    // Read next byte from buffer
    reader->current_byte = reader->buffer[reader->buffer_pos++];
    reader->bit_pos = 0;
    return true;
}