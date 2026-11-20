#include "decompression.h"

#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

namespace easycompress {
namespace {

const std::string FILE_HEADER = "EASYCOMPRESS-RLE-1\n";

int hexValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    return -1;
}

bool parseRunLength(const std::string& encoded,
                    std::size_t& position,
                    std::size_t& runLength) {
    if (position >= encoded.size() ||
        encoded[position] < '0' || encoded[position] > '9') {
        return false;
    }

    runLength = 0;
    bool foundDigit = false;

    while (position < encoded.size() &&
           encoded[position] >= '0' && encoded[position] <= '9') {
        foundDigit = true;
        const std::size_t digit =
            static_cast<std::size_t>(encoded[position] - '0');

        if (runLength > (std::numeric_limits<std::size_t>::max() - digit) / 10) {
            return false;
        }

        runLength = runLength * 10 + digit;
        ++position;
    }

    return foundDigit && runLength > 0;
}

} // namespace

bool decompressRLE(const std::string& encoded,
                   std::string& decoded,
                   std::string& errorMessage) {
    decoded.clear();
    errorMessage.clear();

    if (encoded.compare(0, FILE_HEADER.size(), FILE_HEADER) != 0) {
        errorMessage = "Invalid EASYCOMPRESS file header or unsupported format version.";
        return false;
    }

    std::size_t position = FILE_HEADER.size();

    try {
        while (position < encoded.size()) {
            std::size_t runLength = 0;
            if (!parseRunLength(encoded, position, runLength)) {
                errorMessage = "Invalid run length in compressed data.";
                decoded.clear();
                return false;
            }

            if (position >= encoded.size() || encoded[position] != ':') {
                errorMessage = "Missing ':' separator after a run length.";
                decoded.clear();
                return false;
            }
            ++position;

            if (encoded.size() - position < 3) {
                errorMessage = "Incomplete encoded byte. Each byte needs two hex digits and ';'.";
                decoded.clear();
                return false;
            }

            const int high = hexValue(encoded[position]);
            const int low = hexValue(encoded[position + 1]);
            if (high < 0 || low < 0) {
                errorMessage = "Invalid hexadecimal byte value in compressed data.";
                decoded.clear();
                return false;
            }
            position += 2;

            if (encoded[position] != ';') {
                errorMessage = "Missing ';' record terminator in compressed data.";
                decoded.clear();
                return false;
            }
            ++position;

            if (runLength > decoded.max_size() - decoded.size()) {
                errorMessage = "Decoded data would exceed the maximum supported string size.";
                decoded.clear();
                return false;
            }

            const char byte = static_cast<char>((high << 4) | low);
            decoded.append(runLength, byte);
        }
    } catch (const std::length_error&) {
        errorMessage = "Decoded data is too large to fit in memory.";
        decoded.clear();
        return false;
    } catch (const std::bad_alloc&) {
        errorMessage = "Not enough memory to decompress this file.";
        decoded.clear();
        return false;
    }

    return true;
}

} // namespace easycompress
