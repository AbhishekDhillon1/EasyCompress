#ifndef EASYCOMPRESS_FILE_HANDLER_H
#define EASYCOMPRESS_FILE_HANDLER_H

#include <cstdint>
#include <string>

namespace easycompress {

struct CompressionStats {
    long double spaceSavedBytes;
    double compressionPercentage;
    double compressionRatio;
    bool hasCompressionRatio;
    bool reducedFileSize;
};

// Reads and writes files as raw bytes so that line endings and all byte values
// are preserved exactly.
bool readFileBinary(const std::string& filePath,
                    std::string& contents,
                    std::string& errorMessage);

// Refuses to overwrite an existing path.
bool writeFileBinary(const std::string& filePath,
                     const std::string& contents,
                     std::string& errorMessage);

bool getFileSizeBytes(const std::string& filePath,
                      std::uintmax_t& fileSize,
                      std::string& errorMessage);

// Compares two files in binary mode using fixed-size character arrays.
bool compareFilesByteByByte(const std::string& firstPath,
                            const std::string& secondPath,
                            bool& areIdentical,
                            std::string& errorMessage);

// Finds an unused output name. Example: notes_compressed.rle,
// notes_compressed_1.rle, and so on.
bool makeUniqueOutputPath(const std::string& inputPath,
                          const std::string& suffix,
                          const std::string& extension,
                          std::string& outputPath,
                          std::string& errorMessage);

CompressionStats calculateCompressionStats(std::uintmax_t originalSize,
                                           std::uintmax_t compressedSize);

void displayCompressionStats(const CompressionStats& stats,
                             std::uintmax_t originalSize,
                             std::uintmax_t compressedSize);

} // namespace easycompress

#endif // EASYCOMPRESS_FILE_HANDLER_H