#include "compression.h"
#include "decompression.h"
#include "file_handler.h"
#include "history.h"
#include "structures.h"

#include <cctype>
#include <iostream>
#include <string>
#include <vector>

namespace {

const std::string HISTORY_FILE = "history.txt";
bool inputEnded = false;

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    if (!std::getline(std::cin, input)) {
        inputEnded = true;
    }
    return input;
}

std::string cleanPath(std::string path) {
    // Allow paths copied from File Explorer with surrounding double quotes.
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        path = path.substr(1, path.size() - 2);
    }
    return path;
}

bool askYesNo(const std::string& prompt) {
    while (true) {
        const std::string answer = readLine(prompt + " (y/n): ");
        if (inputEnded) {
            return false;
        }
        if (answer == "y" || answer == "Y" || answer == "yes" || answer == "YES") {
            return true;
        }
        if (answer == "n" || answer == "N" || answer == "no" || answer == "NO") {
            return false;
        }
        std::cout << "Please enter y or n.\n";
    }
}

std::string readMultilineText(const std::string& heading) {
    std::cout << heading << '\n';
    std::cout << "Enter your text one line at a time. Enter .END on its own line to finish.\n";

    std::string text;
    std::string line;
    bool firstLine = true;

    while (std::getline(std::cin, line)) {
        if (line == ".END") {
            break;
        }

        if (!firstLine) {
            text += '\n';
        }
        text += line;
        firstLine = false;
    }
    if (!std::cin && std::cin.eof()) {
        inputEnded = true;
    }

    return text;
}

int readMenuChoice() {
    const std::string input = readLine("\nEnter your choice: ");
    if (input.empty()) {
        return -1;
    }

    try {
        std::size_t parsedCharacters = 0;
        const int choice = std::stoi(input, &parsedCharacters);
        while (parsedCharacters < input.size() &&
               std::isspace(static_cast<unsigned char>(input[parsedCharacters]))) {
            ++parsedCharacters;
        }
        if (parsedCharacters != input.size()) {
            return -1;
        }
        return choice;
    } catch (...) {
        return -1;
    }
}

void printMenu() {
    std::cout << "\n====================================\n";
    std::cout << "             EASYCOMPRESS\n";
    std::cout << "       FILE COMPRESSION SYSTEM\n";
    std::cout << "====================================\n";
    std::cout << "1.  Compress Text\n";
    std::cout << "2.  Decompress Text\n";
    std::cout << "3.  Compress File\n";
    std::cout << "4.  Decompress File\n";
    std::cout << "5.  Compare Two Files\n";
    std::cout << "6.  View Compression History\n";
    std::cout << "7.  Search History\n";
    std::cout << "8.  Add File to Compression Queue\n";
    std::cout << "9.  Process Compression Queue\n";
    std::cout << "10. Undo Last Text Operation\n";
    std::cout << "11. Exit\n";
    std::cout << "====================================\n";
}

void handleCompressText(easycompress::TextOperationStack& undoStack) {
    const std::string originalText = readMultilineText("\n========== TEXT COMPRESSION ==========");
    const std::string compressedText = easycompress::compressRLE(originalText);

    std::cout << "\nCompressed output:\n";
    std::cout << compressedText << '\n';

    easycompress::TextOperation operation;
    operation.originalText = originalText;
    operation.compressedText = compressedText;
    undoStack.push(operation);
    std::cout << "\nText compression operation stored in the undo stack.\n";

    if (askYesNo("Save this compressed output to a .rle file?")) {
        std::string outputPath;
        std::string errorMessage;
        if (!easycompress::makeUniqueOutputPath("manual_text.txt",
                                                "_compressed",
                                                ".rle",
                                                outputPath,
                                                errorMessage)) {
            std::cout << "Error: " << errorMessage << '\n';
            return;
        }

        if (!easycompress::writeFileBinary(outputPath, compressedText, errorMessage)) {
            std::cout << "Error: " << errorMessage << '\n';
            return;
        }
        std::cout << "Compressed text saved to: " << outputPath << '\n';
    }
}

void handleDecompressText() {
    const std::string encodedText = readMultilineText("\n========== TEXT DECOMPRESSION ==========");
    std::string decodedText;
    std::string errorMessage;

    if (!easycompress::decompressRLE(encodedText, decodedText, errorMessage)) {
        std::cout << "Decompression failed: " << errorMessage << '\n';
        return;
    }

    std::cout << "\nDecompressed text:\n";
    std::cout.write(decodedText.data(), static_cast<std::streamsize>(decodedText.size()));
    std::cout << "\n\nDecompression completed successfully.\n";
}

bool compressFile(const std::string& inputPath,
                  std::vector<easycompress::HistoryRecord>& historyRecords) {
    std::string originalContents;
    std::string errorMessage;

    if (!easycompress::readFileBinary(inputPath, originalContents, errorMessage)) {
        std::cout << "Error: " << errorMessage << '\n';
        return false;
    }

    std::string outputPath;
    if (!easycompress::makeUniqueOutputPath(inputPath,
                                            "_compressed",
                                            ".rle",
                                            outputPath,
                                            errorMessage)) {
        std::cout << "Error: " << errorMessage << '\n';
        return false;
    }

    const std::string compressedContents = easycompress::compressRLE(originalContents);
    if (!easycompress::writeFileBinary(outputPath, compressedContents, errorMessage)) {
        std::cout << "Error: " << errorMessage << '\n';
        return false;
    }

    std::uintmax_t compressedSize = 0;
    if (!easycompress::getFileSizeBytes(outputPath, compressedSize, errorMessage)) {
        std::cout << "Warning: compressed file was written, but its size could not be read: "
                  << errorMessage << '\n';
        compressedSize = static_cast<std::uintmax_t>(compressedContents.size());
    }

    const std::uintmax_t originalSize =
        static_cast<std::uintmax_t>(originalContents.size());
    const easycompress::CompressionStats stats =
        easycompress::calculateCompressionStats(originalSize, compressedSize);

    std::cout << "\nFile compressed successfully.\n";
    std::cout << "Compressed file: " << outputPath << '\n';
    easycompress::displayCompressionStats(stats, originalSize, compressedSize);

    easycompress::HistoryRecord record{};
    record.originalFilename = inputPath;
    record.compressedFilename = outputPath;
    record.originalSize = originalSize;
    record.compressedSize = compressedSize;
    record.compressionPercentage = stats.compressionPercentage;

    if (easycompress::appendHistory(HISTORY_FILE, record, errorMessage)) {
        historyRecords.push_back(record);
        std::cout << "Compression operation added to history.\n";
    } else {
        std::cout << "Warning: compression succeeded, but history could not be saved: "
                  << errorMessage << '\n';
    }

    return true;
}

void handleCompressFile(std::vector<easycompress::HistoryRecord>& historyRecords) {
    const std::string inputPath = cleanPath(readLine("Enter the input text file path: "));
    if (inputPath.empty()) {
        std::cout << "No input path was provided.\n";
        return;
    }
    compressFile(inputPath, historyRecords);
}

void handleDecompressFile() {
    const std::string inputPath = cleanPath(readLine("Enter the .rle file path: "));
    if (inputPath.empty()) {
        std::cout << "No input path was provided.\n";
        return;
    }

    std::string encodedContents;
    std::string errorMessage;
    if (!easycompress::readFileBinary(inputPath, encodedContents, errorMessage)) {
        std::cout << "Error: " << errorMessage << '\n';
        return;
    }

    std::string decodedContents;
    if (!easycompress::decompressRLE(encodedContents, decodedContents, errorMessage)) {
        std::cout << "Decompression failed: " << errorMessage << '\n';
        return;
    }

    std::string outputPath;
    if (!easycompress::makeUniqueOutputPath(inputPath,
                                            "_decompressed",
                                            ".txt",
                                            outputPath,
                                            errorMessage)) {
        std::cout << "Error: " << errorMessage << '\n';
        return;
    }

    if (!easycompress::writeFileBinary(outputPath, decodedContents, errorMessage)) {
        std::cout << "Error: " << errorMessage << '\n';
        return;
    }

    std::cout << "\nDecompression completed successfully.\n";
    std::cout << "Restored file saved as: " << outputPath << '\n';
    std::cout << "Restored file size: " << decodedContents.size() << " bytes\n";
}

void handleCompareFiles() {
    const std::string firstPath = cleanPath(readLine("Enter the first file path: "));
    const std::string secondPath = cleanPath(readLine("Enter the second file path: "));

    if (firstPath.empty() || secondPath.empty()) {
        std::cout << "Both file paths are required.\n";
        return;
    }

    bool areIdentical = false;
    std::string errorMessage;
    if (!easycompress::compareFilesByteByByte(firstPath,
                                              secondPath,
                                              areIdentical,
                                              errorMessage)) {
        std::cout << "Comparison failed: " << errorMessage << '\n';
        return;
    }

    if (areIdentical) {
        std::cout << "\nResult: Both files are identical (byte-for-byte).\n";
    } else {
        std::cout << "\nResult: The files are different.\n";
    }
}

void handleViewHistory(const std::vector<easycompress::HistoryRecord>& historyRecords) {
    easycompress::displayHistory(historyRecords);
}

void handleSearchHistory(const std::vector<easycompress::HistoryRecord>& historyRecords) {
    const std::string query = readLine("Enter a filename or part of a filename to search: ");
    if (query.empty()) {
        std::cout << "Search text cannot be empty.\n";
        return;
    }

    const std::vector<easycompress::HistoryRecord> matches =
        easycompress::searchHistory(historyRecords, query);
    if (matches.empty()) {
        std::cout << "No matching history records were found.\n";
    } else {
        std::cout << matches.size() << " matching record(s) found.\n";
        easycompress::displayHistory(matches);
    }
}

void handleAddToQueue(easycompress::CompressionQueue& requestQueue) {
    const std::string inputPath = cleanPath(readLine("Enter a file path to add to the queue: "));
    if (inputPath.empty()) {
        std::cout << "No file path was provided.\n";
        return;
    }

    std::uintmax_t ignoredSize = 0;
    std::string errorMessage;
    if (!easycompress::getFileSizeBytes(inputPath, ignoredSize, errorMessage)) {
        std::cout << "Could not add file: " << errorMessage << '\n';
        return;
    }

    if (!requestQueue.enqueue(inputPath)) {
        std::cout << "The compression queue is full (maximum 100 requests).\n";
        return;
    }

    std::cout << "File added to the queue. Current queue size: "
              << requestQueue.size() << '\n';
}

void handleProcessQueue(easycompress::CompressionQueue& requestQueue,
                        std::vector<easycompress::HistoryRecord>& historyRecords) {
    if (requestQueue.empty()) {
        std::cout << "The compression queue is empty.\n";
        return;
    }

    std::cout << "\nProcessing queued files in FIFO order...\n";
    std::string inputPath;
    std::size_t requestNumber = 1;

    while (requestQueue.dequeue(inputPath)) {
        std::cout << "\n---------- Queue request " << requestNumber << " ----------\n";
        std::cout << "File: " << inputPath << '\n';
        compressFile(inputPath, historyRecords);
        ++requestNumber;
    }

    std::cout << "\nQueue processing finished.\n";
}

void handleUndo(easycompress::TextOperationStack& undoStack) {
    easycompress::TextOperation operation;
    if (!undoStack.pop(operation)) {
        std::cout << "There is no manual text operation to undo.\n";
        return;
    }

    std::cout << "\nUndo completed. The most recent manual text operation was removed\n";
    std::cout << "from the in-memory undo stack. Original text:\n";
    std::cout.write(operation.originalText.data(),
                    static_cast<std::streamsize>(operation.originalText.size()));
    std::cout << "\n\nRemaining undo operations: " << undoStack.size() << '\n';
    std::cout << "Note: Undo does not delete a .rle file that was already saved.\n";
}

} // namespace

int main() {
    std::vector<easycompress::HistoryRecord> historyRecords;
    std::string historyMessage;
    if (!easycompress::loadHistory(HISTORY_FILE, historyRecords, historyMessage)) {
        std::cout << "History warning: " << historyMessage << '\n';
    } else if (!historyMessage.empty()) {
        std::cout << "History notice: " << historyMessage << '\n';
    }

    easycompress::TextOperationStack undoStack;
    easycompress::CompressionQueue requestQueue;

    std::cout << "Welcome to EASYCOMPRESS!\n";

    bool running = true;
    while (running && !inputEnded) {
        printMenu();
        const int choice = readMenuChoice();
        if (inputEnded) {
            break;
        }

        switch (choice) {
            case 1:
                handleCompressText(undoStack);
                break;
            case 2:
                handleDecompressText();
                break;
            case 3:
                handleCompressFile(historyRecords);
                break;
            case 4:
                handleDecompressFile();
                break;
            case 5:
                handleCompareFiles();
                break;
            case 6:
                handleViewHistory(historyRecords);
                break;
            case 7:
                handleSearchHistory(historyRecords);
                break;
            case 8:
                handleAddToQueue(requestQueue);
                break;
            case 9:
                handleProcessQueue(requestQueue, historyRecords);
                break;
            case 10:
                handleUndo(undoStack);
                break;
            case 11:
                running = false;
                std::cout << "Thank you for using EASYCOMPRESS. Goodbye!\n";
                break;
            default:
                std::cout << "Invalid choice. Please enter a number from 1 to 11.\n";
                break;
        }
        if (inputEnded) {
            running = false;
        }
    }

    return 0;
}
