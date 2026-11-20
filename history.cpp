#include "history.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace easycompress {
namespace {

const char* const HISTORY_HEADER = "EASYCOMPRESS_HISTORY_1";
const char* const HEX_DIGITS = "0123456789ABCDEF";

std::string encodeHex(const std::string& value) {
    std::string encoded;
    encoded.reserve(value.size() * 2);

    for (unsigned char byte : value) {
        encoded += HEX_DIGITS[(byte >> 4) & 0x0F];
        encoded += HEX_DIGITS[byte & 0x0F];
    }
    return encoded;
}

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

bool decodeHex(const std::string& encoded, std::string& decoded) {
    if (encoded.size() % 2 != 0) {
        return false;
    }

    decoded.clear();
    decoded.reserve(encoded.size() / 2);
    for (std::size_t index = 0; index < encoded.size(); index += 2) {
        const int high = hexValue(encoded[index]);
        const int low = hexValue(encoded[index + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        decoded.push_back(static_cast<char>((high << 4) | low));
    }
    return true;
}

std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;

    while (true) {
        const std::size_t separator = line.find('\t', start);
        if (separator == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, separator - start));
        start = separator + 1;
    }
    return fields;
}

bool parseUnsigned(const std::string& text, std::uintmax_t& value) {
    if (text.empty()) {
        return false;
    }

    std::istringstream input(text);
    unsigned long long parsed = 0;
    input >> parsed;
    if (!input || !input.eof()) {
        return false;
    }
    value = static_cast<std::uintmax_t>(parsed);
    return true;
}

std::string toLowerCopy(const std::string& value) {
    std::string result = value;
    for (char& character : result) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return result;
}

} // namespace

bool loadHistory(const std::string& historyPath,
                 std::vector<HistoryRecord>& records,
                 std::string& warningMessage) {
    records.clear();
    warningMessage.clear();

    std::error_code filesystemError;
    const bool exists = std::filesystem::exists(historyPath, filesystemError);
    if (filesystemError) {
        warningMessage = "Could not inspect history file: " + filesystemError.message();
        return false;
    }
    if (!exists) {
        return true;
    }

    std::ifstream input(historyPath, std::ios::binary);
    if (!input.is_open()) {
        warningMessage = "Could not open history file: " + historyPath;
        return false;
    }

    std::string firstLine;
    if (!std::getline(input, firstLine)) {
        return true;
    }
    if (firstLine != HISTORY_HEADER) {
        warningMessage = "The history file has an unknown format. Existing records were not loaded.";
        return false;
    }

    std::string line;
    std::size_t invalidRecords = 0;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> fields = splitTabs(line);
        if (fields.size() != 5) {
            ++invalidRecords;
            continue;
        }

        HistoryRecord record{};
        if (!decodeHex(fields[0], record.originalFilename) ||
            !decodeHex(fields[1], record.compressedFilename) ||
            !parseUnsigned(fields[2], record.originalSize) ||
            !parseUnsigned(fields[3], record.compressedSize)) {
            ++invalidRecords;
            continue;
        }

        try {
            std::size_t parsedCharacters = 0;
            record.compressionPercentage = std::stod(fields[4], &parsedCharacters);
            if (parsedCharacters != fields[4].size()) {
                ++invalidRecords;
                continue;
            }
        } catch (...) {
            ++invalidRecords;
            continue;
        }

        records.push_back(record);
    }

    if (invalidRecords > 0) {
        warningMessage = "Loaded " + std::to_string(records.size()) +
                         " valid history record(s); skipped " +
                         std::to_string(invalidRecords) + " invalid record(s).";
    }

    return true;
}

bool appendHistory(const std::string& historyPath,
                   const HistoryRecord& record,
                   std::string& errorMessage) {
    errorMessage.clear();

    std::error_code filesystemError;
    bool needsHeader = !std::filesystem::exists(historyPath, filesystemError);
    if (filesystemError) {
        errorMessage = "Could not inspect history file: " + filesystemError.message();
        return false;
    }

    if (!needsHeader) {
        const std::uintmax_t size = std::filesystem::file_size(historyPath, filesystemError);
        if (filesystemError) {
            errorMessage = "Could not inspect history file size: " + filesystemError.message();
            return false;
        }
        needsHeader = size == 0;
    }

    std::ofstream output(historyPath, std::ios::binary | std::ios::app);
    if (!output.is_open()) {
        errorMessage = "Could not open history file for writing: " + historyPath;
        return false;
    }

    if (needsHeader) {
        output << HISTORY_HEADER << '\n';
    }

    output << encodeHex(record.originalFilename) << '\t'
           << encodeHex(record.compressedFilename) << '\t'
           << record.originalSize << '\t'
           << record.compressedSize << '\t'
           << std::setprecision(17) << record.compressionPercentage << '\n';

    output.flush();
    if (!output) {
        errorMessage = "Could not save the compression history record.";
        return false;
    }

    return true;
}

void displayHistory(const std::vector<HistoryRecord>& records) {
    std::cout << "\n================ COMPRESSION HISTORY ================\n";
    if (records.empty()) {
        std::cout << "No compression operations have been recorded yet.\n";
        std::cout << "======================================================\n";
        return;
    }

    for (std::size_t index = 0; index < records.size(); ++index) {
        const HistoryRecord& record = records[index];
        std::cout << "\nRecord " << (index + 1) << '\n';
        std::cout << "  Original file   : " << record.originalFilename << '\n';
        std::cout << "  Compressed file : " << record.compressedFilename << '\n';
        std::cout << "  Original size   : " << record.originalSize << " bytes\n";
        std::cout << "  Compressed size : " << record.compressedSize << " bytes\n";
        std::cout << "  Space saved (%) : " << std::fixed << std::setprecision(2)
                  << record.compressionPercentage << "%\n";
    }
    std::cout << "\n======================================================\n";
}

std::vector<HistoryRecord> searchHistory(const std::vector<HistoryRecord>& records,
                                         const std::string& filenameQuery) {
    std::vector<HistoryRecord> matches;
    const std::string query = toLowerCopy(filenameQuery);

    for (const HistoryRecord& record : records) {
        const std::string original = toLowerCopy(record.originalFilename);
        const std::string compressed = toLowerCopy(record.compressedFilename);
        if (original.find(query) != std::string::npos ||
            compressed.find(query) != std::string::npos) {
            matches.push_back(record);
        }
    }

    return matches;
}

} // namespace easycompress
