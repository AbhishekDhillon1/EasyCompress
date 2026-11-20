#ifndef EASYCOMPRESS_HISTORY_H
#define EASYCOMPRESS_HISTORY_H

#include <cstdint>
#include <string>
#include <vector>

namespace easycompress {

struct HistoryRecord {
    std::string originalFilename;
    std::string compressedFilename;
    std::uintmax_t originalSize;
    std::uintmax_t compressedSize;
    double compressionPercentage;
};

// Loads records from a persistent text file. A missing history file is valid
// and produces an empty list.
bool loadHistory(const std::string& historyPath,
                 std::vector<HistoryRecord>& records,
                 std::string& warningMessage);

bool appendHistory(const std::string& historyPath,
                   const HistoryRecord& record,
                   std::string& errorMessage);

void displayHistory(const std::vector<HistoryRecord>& records);

std::vector<HistoryRecord> searchHistory(const std::vector<HistoryRecord>& records,
                                         const std::string& filenameQuery);

} // namespace easycompress

#endif // EASYCOMPRESS_HISTORY_H