# Huffman compression

This is an attempt to implement a compression algorithm using the simple [Huffman codes](https://en.wikipedia.org/wiki/Huffman_coding).
This is a lossless data compression algorithm based on optimal prefix.

## Compile & run

Since this is a small project, I did not use cmake, so you have to compile it using `gcc *.c IO/*.c huffman/*.c -o huffman.exe`.

To run it, just use `./hufffman` and it will show how to properly run it.

## How it works

This code implements :

- A general-purpose (`void *`) minimum priority queue
- Stream reader and writer each capable of working on single bits
- An Huffman tree builder, encoder and decoder

All of these are 1st class ADTs.

And then the main, which just provides the interface to these ADTs.

## Limits

The Huffman codes works well with a lot of the same data, but with small amounts of varying data it might even output a larger file.
