#include "bit_stream_writer.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUFFER_SIZE 1024

struct BitStreamWriter
{
    FILE *file;           // Output file
    uint8_t *buffer;      // Buffer for storing bytes before writing to file
    size_t buffer_size;   // Size of the buffer in bytes
    size_t buffer_pos;    // Current position in the buffer
    uint8_t current_byte; // Byte being currently constructed
    uint8_t bit_pos;      // Position within the current byte (0-7)
    uint64_t total_bits;  // Total number of bits written
    bool has_error;       // Error flag
    bool close_on_free;   // Whether to close the file when freeing
};

static bool flush_buffer(BitSW writer);
static bool add_byte_to_buffer(BitSW writer, uint8_t byte);

BitSW bsw_create(const char *filename, size_t buffer_size)
{
    if (!filename)
        return NULL;

    BitSW writer = bsw_create_from_file(filename, buffer_size, true);
    return writer;
}

BitSW bsw_create_from_file(const char *filename, size_t buffer_size, bool close_on_free)
{
    if (!filename)
        return NULL;

    FILE *file = fopen(filename, "wb");
    if (!file)
        return NULL;

    BitSW writer = (BitSW)malloc(sizeof *writer);
    if (!writer)
    {
        fclose(file);
        return NULL;
    }

    if (buffer_size == 0)
        buffer_size = DEFAULT_BUFFER_SIZE;

    writer->buffer = (uint8_t *)malloc(buffer_size);
    if (!writer->buffer)
    {
        free(writer);
        return NULL;
    }

    writer->file = file;
    writer->buffer_size = buffer_size;
    writer->buffer_pos = 0;
    writer->current_byte = 0;
    writer->bit_pos = 0;
    writer->total_bits = 0;
    writer->has_error = false;
    writer->close_on_free = close_on_free;

    return writer;
}

void bsw_free(BitSW writer)
{
    if (!writer)
        return;

    // Flush any pending bits
    bsw_flush(writer);

    // Free resources
    if (writer->file && writer->close_on_free)
        fclose(writer->file);

    free(writer->buffer);
    free(writer);
}

bool bsw_write_bit(BitSW writer, uint8_t bit)
{
    if (!writer || writer->has_error)
        return false;

    // Set the bit in the current byte
    if (bit)
        writer->current_byte |= (1 << (7 - writer->bit_pos));

    // Increment bit position
    writer->bit_pos++;
    writer->total_bits++;

    // If we've completed a byte, add it to the buffer
    if (writer->bit_pos == 8)
    {
        if (!add_byte_to_buffer(writer, writer->current_byte))
            return false;

        writer->current_byte = 0;
        writer->bit_pos = 0;
    }

    return true;
}

bool bsw_write_bits(BitSW writer, uint8_t *values, uint8_t num_bits)
{
    if (!writer || !values || writer->has_error || num_bits > 64)
        return false;

    // Write bits from most significant to least significant
    for (int i = 0; i < num_bits; i++)
    {
        uint8_t bit = (values[i / 8] >> (7 - (i % 8))) & 1;
        if (!bsw_write_bit(writer, bit))
            return false;
    }

    return true;
}

bool bsw_write_byte(BitSW writer, uint8_t byte)
{
    if (writer->bit_pos == 0)
    {
        bool res = add_byte_to_buffer(writer, byte);
        if (!res)
            return false;

        writer->total_bits += 8;
        return true;
    }

    uint8_t bits[8];
    for (int i = 0; i < 8; i++)
        bits[i] = (byte >> (7 - i)) & 1;
    return bsw_write_bits(writer, bits, 8);
}

bool bsw_write_bytes(BitSW writer, uint8_t *data, size_t size)
{
    if (!writer || !data || writer->has_error)
        return false;

    // If we're at a byte boundary and have a lot of data, use optimized path
    if (writer->bit_pos == 0 && size > 8)
    {
        // Flush any existing buffer content
        if (writer->buffer_pos > 0 && !flush_buffer(writer))
            return false;

        // Write directly to the file if data is larger than buffer
        if (size >= writer->buffer_size)
        {
            size_t bytes_written = fwrite(data, 1, size, writer->file);
            if (bytes_written != size)
            {
                writer->has_error = true;
                return false;
            }
            writer->total_bits += (size * 8);
            return true;
        }

        // Otherwise, copy to the buffer
        memcpy(writer->buffer, data, size);
        writer->buffer_pos = size;
        writer->total_bits += (size * 8);
        return true;
    }

    // For small data or non-byte-aligned positions, write byte by byte
    for (size_t i = 0; i < size; i++)
        if (!bsw_write_byte(writer, data[i]))
            return false;

    return true;
}

bool bsw_flush(BitSW writer)
{
    if (!writer)
        return false;

    // If there are bits pending in the current byte, pad with zeros and write
    if (writer->bit_pos > 0)
    {
        if (!add_byte_to_buffer(writer, writer->current_byte))
            return false;
        writer->current_byte = 0;
        writer->bit_pos = 0;
    }

    // Flush the buffer to the file
    if (!flush_buffer(writer))
        return false;

    // Flush the file
    if (fflush(writer->file) != 0)
    {
        writer->has_error = true;
        return false;
    }

    return true;
}

bool bsw_align_to_byte(BitSW writer)
{
    if (!writer)
        return false;

    // If already aligned, nothing to do
    if (writer->bit_pos == 0)
        return true;

    // Add the partially filled byte to the buffer
    if (!add_byte_to_buffer(writer, writer->current_byte))
        return false;

    // Reset for the next byte
    writer->total_bits += 8 - writer->bit_pos;
    writer->current_byte = 0;
    writer->bit_pos = 0;

    return true;
}

uint64_t bsw_get_bits_written(BitSW writer)
{
    if (!writer)
        return 0;

    return writer->total_bits;
}

uint64_t bsw_get_bytes_written(BitSW writer)
{
    if (!writer)
        return 0;

    return (writer->total_bits / 8) + (writer->buffer_pos);
}

FILE *bsw_get_file(BitSW writer)
{
    if (!writer)
        return NULL;

    return writer->file;
}

bool bsw_has_error(BitSW writer)
{
    if (!writer)
        return true;

    return writer->has_error;
}

void bsw_clear_error(BitSW writer)
{
    if (writer)
        writer->has_error = false;
}

///// Static Functions /////

// Helper function to write the buffer to the file
static bool flush_buffer(BitSW writer)
{
    if (writer->buffer_pos == 0)
        return true; // Nothing to flush

    size_t bytes_written = fwrite(writer->buffer, 1, writer->buffer_pos, writer->file);
    if (bytes_written != writer->buffer_pos)
    {
        writer->has_error = true;
        return false;
    }

    writer->buffer_pos = 0;
    return true;
}

// Helper function to add a byte to the buffer
static bool add_byte_to_buffer(BitSW writer, uint8_t byte)
{
    if (writer->buffer_pos >= writer->buffer_size)
        if (!flush_buffer(writer))
            return false;

    writer->buffer[writer->buffer_pos++] = byte;
    return true;
}