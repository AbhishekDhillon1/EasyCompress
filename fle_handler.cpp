#include "file_handler.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

namespace easycompress {

bool readFileBinary(const std::string& filePath,
                    std::string& contents,
                    std::string& errorMessage) {
    contents.clear();
    errorMessage.clear();

    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        errorMessage = "Could not open input file: " + filePath;
        return false;
    }

    input.seekg(0, std::ios::end);
    const std::streampos endPosition = input.tellg();
    if (endPosition < 0) {
        errorMessage = "Could not determine the size of: " + filePath;
        return false;
    }

    input.seekg(0, std::ios::beg);
    if (!input) {
        errorMessage = "Could not read input file: " + filePath;
        return false;
    }

    const std::streamoff length = static_cast<std::streamoff>(endPosition);
    if (length > static_cast<std::streamoff>(
                     std::numeric_limits<std::streamsize>::max())) {
        errorMessage = "The input file is too large for a single file read operation.";
        return false;
    }
    if (length > 0) {
        if (static_cast<unsigned long long>(length) >
            static_cast<unsigned long long>(contents.max_size())) {
            errorMessage = "The input file is too large for this application to load into memory.";
            return false;
        }

        contents.resize(static_cast<std::size_t>(length));
        input.read(&contents[0], static_cast<std::streamsize>(length));
        if (!input || input.gcount() != static_cast<std::streamsize>(length)) {
            contents.clear();
            errorMessage = "A read error occurred while reading: " + filePath;
            return false;
        }
    }

    return true;
}

bool writeFileBinary(const std::string& filePath,
                     const std::string& contents,
                     std::string& errorMessage) {
    errorMessage.clear();

    if (filePath.empty()) {
        errorMessage = "Output file path cannot be empty.";
        return false;
    }

    std::error_code filesystemError;
    const bool alreadyExists = std::filesystem::exists(filePath, filesystemError);
    if (filesystemError) {
        errorMessage = "Could not check output path: " + filesystemError.message();
        return false;
    }
    if (alreadyExists) {
        errorMessage = "The output file already exists. It was not overwritten: " + filePath;
        return false;
    }

    std::ofstream output(filePath, std::ios::binary | std::ios::out);
    if (!output.is_open()) {
        errorMessage = "Could not create output file: " + filePath;
        return false;
    }

    const std::size_t chunkSize = 1024 * 1024;
    std::size_t position = 0;
    while (position < contents.size()) {
        const std::size_t remaining = contents.size() - position;
        const std::size_t amount = std::min(chunkSize, remaining);
        output.write(contents.data() + position,
                     static_cast<std::streamsize>(amount));
        if (!output) {
            output.close();
            errorMessage = "A write error occurred while creating: " + filePath;
            return false;
        }
        position += amount;
    }

    output.flush();
    if (!output) {
        output.close();
        errorMessage = "Could not finish writing: " + filePath;
        return false;
    }

    return true;
}

bool getFileSizeBytes(const std::string& filePath,
                      std::uintmax_t& fileSize,
                      std::string& errorMessage) {
    errorMessage.clear();
    std::error_code filesystemError;
    fileSize = std::filesystem::file_size(filePath, filesystemError);
    if (filesystemError) {
        errorMessage = "Could not get file size for '" + filePath + "': " +
                       filesystemError.message();
        return false;
    }
    errorMessage.clear();
    return true;
}

bool compareFilesByteByByte(const std::string& firstPath,
                            const std::string& secondPath,
                            bool& areIdentical,
                            std::string& errorMessage) {
    areIdentical = false;
    errorMessage.clear();

    std::ifstream firstFile(firstPath, std::ios::binary);
    if (!firstFile.is_open()) {
        errorMessage = "Could not open first file: " + firstPath;
        return false;
    }

    std::ifstream secondFile(secondPath, std::ios::binary);
    if (!secondFile.is_open()) {
        errorMessage = "Could not open second file: " + secondPath;
        return false;
    }

    std::array<char, 4096> firstBuffer{};
    std::array<char, 4096> secondBuffer{};

    while (true) {
        firstFile.read(firstBuffer.data(),
                       static_cast<std::streamsize>(firstBuffer.size()));
        secondFile.read(secondBuffer.data(),
                        static_cast<std::streamsize>(secondBuffer.size()));

        const std::streamsize firstCount = firstFile.gcount();
        const std::streamsize secondCount = secondFile.gcount();

        if (firstFile.bad() || secondFile.bad()) {
            errorMessage = "A read error occurred during file comparison.";
            return false;
        }

        if (firstCount != secondCount) {
            areIdentical = false;
            return true;
        }

        if (!std::equal(firstBuffer.begin(),
                        firstBuffer.begin() + firstCount,
                        secondBuffer.begin())) {
            areIdentical = false;
            return true;
        }

        if (firstCount == 0) {
            areIdentical = true;
            return true;
        }
    }
}

bool makeUniqueOutputPath(const std::string& inputPath,
                          const std::string& suffix,
                          const std::string& extension,
                          std::string& outputPath,
                          std::string& errorMessage) {
    outputPath.clear();
    errorMessage.clear();

    const std::filesystem::path sourcePath(inputPath);
    const std::filesystem::path parent = sourcePath.parent_path();
    const std::string baseName = sourcePath.stem().string();
    const std::string finalExtension = extension.empty() || extension[0] == '.'
                                           ? extension
                                           : "." + extension;

    for (unsigned int index = 0; index < 100000; ++index) {
        std::string candidateName = baseName + suffix;
        if (index > 0) {
            candidateName += "_" + std::to_string(index);
        }
        candidateName += finalExtension;

        const std::filesystem::path candidate = parent / candidateName;
        std::error_code filesystemError;
        const bool exists = std::filesystem::exists(candidate, filesystemError);
        if (filesystemError) {
            errorMessage = "Could not inspect output directory: " + filesystemError.message();
            return false;
        }

        if (!exists) {
            outputPath = candidate.string();
            return true;
        }
    }

    errorMessage = "Could not find an unused output filename.";
    return false;
}

CompressionStats calculateCompressionStats(std::uintmax_t originalSize,
                                           std::uintmax_t compressedSize) {
    CompressionStats stats{};
    stats.spaceSavedBytes = static_cast<long double>(originalSize) -
                            static_cast<long double>(compressedSize);
    stats.compressionPercentage = 0.0;
    stats.compressionRatio = 0.0;
    stats.hasCompressionRatio = compressedSize != 0;
    stats.reducedFileSize = compressedSize < originalSize;

    if (originalSize != 0) {
        stats.compressionPercentage = static_cast<double>(
            (stats.spaceSavedBytes / static_cast<long double>(originalSize)) * 100.0L);
    }

    if (compressedSize != 0) {
        stats.compressionRatio = static_cast<double>(originalSize) /
                                 static_cast<double>(compressedSize);
    }

    return stats;
}

void displayCompressionStats(const CompressionStats& stats,
                             std::uintmax_t originalSize,
                             std::uintmax_t compressedSize) {
    std::cout << "\n========== COMPRESSION STATISTICS ==========" << '\n';
    std::cout << "Original file size   : " << originalSize << " bytes\n";
    std::cout << "Compressed file size : " << compressedSize << " bytes\n";
    std::cout << "Space saved          : " << std::fixed << std::setprecision(0)
              << stats.spaceSavedBytes << " bytes\n";
    std::cout << "Compression percent  : " << std::fixed << std::setprecision(2)
              << stats.compressionPercentage << "%\n";

    std::cout << "Compression ratio    : ";
    if (stats.hasCompressionRatio) {
        std::cout << std::fixed << std::setprecision(2)
                  << stats.compressionRatio << ":1\n";
    } else {
        std::cout << "N/A (compressed size is zero)\n";
    }

    std::cout << "Reduced file size    : "
              << (stats.reducedFileSize ? "Yes" : "No") << '\n';
    if (originalSize == 0) {
        std::cout << "Note                 : Original file is empty; percentage is shown as 0.00%.\n";
    }
    std::cout << "============================================\n";
}

} // namespace easycompress
