#pragma once

#include <cstddef> // for size_t

// google snappy based compressor
// faster decompression [+]
// better compression ratio [+]
// snappy incompatible binary format [-]
// lower compression speed [-]
// all data arrays must have at least 32 unused bytes after data + size
size_t YappyUnCompress(const char* data, size_t size, char* to);
size_t YappyCompress(const char* data, size_t size, char* to);
