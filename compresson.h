#ifndef EASYCOMPRESS_COMPRESSION_H
#define EASYCOMPRESS_COMPRESSION_H

#include <string>

namespace easycompress {

// Compresses a byte string using Run-Length Encoding (RLE).
// The returned data contains the EASYCOMPRESS-RLE-1 header and encoded records.
std::string compressRLE(const std::string& input);

} // namespace easycompress

#endif // EASYCOMPRESS_COMPRESSION_H