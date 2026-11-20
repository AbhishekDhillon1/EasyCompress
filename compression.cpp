#include "compression.h"

#include <cstddef>

namespace easycompress {
namespace {

const char* const FILE_HEADER = "EASYCOMPRESS-RLE-1\n";

} // namespace

std::string compressRLE(const std::string& input) {
    std::string encoded = FILE_HEADER;

    std::size_t index = 0;
    while (index < input.size()) {
        const unsigned char currentByte =
            static_cast<unsigned char>(input[index]);

        std::size_t runLength = 1;
        while (index + runLength < input.size() &&
               static_cast<unsigned char>(input[index + runLength]) == currentByte) {
            ++runLength;
        }

        // Each record is: decimal count, colon, two hexadecimal digits, semicolon.
        encoded += std::to_string(runLength);
        encoded += ':';
        static const char HEX_DIGITS[] = "0123456789ABCDEF";
        encoded += HEX_DIGITS[(currentByte >> 4) & 0x0F];
        encoded += HEX_DIGITS[currentByte & 0x0F];
        encoded += ';';

        index += runLength;
    }

    return encoded;
}

} // namespace easycompress
