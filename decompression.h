#ifndef EASYCOMPRESS_DECOMPRESSION_H
#define EASYCOMPRESS_DECOMPRESSION_H

#include <string>

namespace easycompress {

// Decodes data produced by compressRLE().
// Returns false and fills errorMessage when the input is invalid.
bool decompressRLE(const std::string& encoded,
                   std::string& decoded,
                   std::string& errorMessage);

} // namespace easycompress

#endif // EASYCOMPRESS_DECOMPRESSION_H