#ifndef EASYCOMPRESS_STRUCTURES_H
#define EASYCOMPRESS_STRUCTURES_H

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace easycompress {

struct TextOperation {
    std::string originalText;
    std::string compressedText;
};

// Stack: the most recent manual text compression operation is undone first.
class TextOperationStack {
public:
    void push(const TextOperation& operation);
    bool pop(TextOperation& operation);
    bool empty() const;
    std::size_t size() const;

private:
    std::vector<TextOperation> items;
};

// Fixed-capacity circular queue for file paths. Requests are processed FIFO.
class CompressionQueue {
public:
    static const std::size_t CAPACITY = 100;

    CompressionQueue();
    bool enqueue(const std::string& filePath);
    bool dequeue(std::string& filePath);
    bool empty() const;
    bool full() const;
    std::size_t size() const;

private:
    std::array<std::string, CAPACITY> items;
    std::size_t frontIndex;
    std::size_t rearIndex;
    std::size_t currentSize;
};

} // namespace easycompress

#endif // EASYCOMPRESS_STRUCTURES_H