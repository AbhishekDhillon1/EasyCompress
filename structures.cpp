#include "structures.h"

#include <utility>

namespace easycompress {

void TextOperationStack::push(const TextOperation& operation) {
    items.push_back(operation);
}

bool TextOperationStack::pop(TextOperation& operation) {
    if (items.empty()) {
        return false;
    }

    operation = std::move(items.back());
    items.pop_back();
    return true;
}

bool TextOperationStack::empty() const {
    return items.empty();
}

std::size_t TextOperationStack::size() const {
    return items.size();
}

CompressionQueue::CompressionQueue()
    : frontIndex(0), rearIndex(0), currentSize(0) {
}

bool CompressionQueue::enqueue(const std::string& filePath) {
    if (full()) {
        return false;
    }

    items[rearIndex] = filePath;
    rearIndex = (rearIndex + 1) % CAPACITY;
    ++currentSize;
    return true;
}

bool CompressionQueue::dequeue(std::string& filePath) {
    if (empty()) {
        return false;
    }

    filePath = std::move(items[frontIndex]);
    items[frontIndex].clear();
    frontIndex = (frontIndex + 1) % CAPACITY;
    --currentSize;
    return true;
}

bool CompressionQueue::empty() const {
    return currentSize == 0;
}

bool CompressionQueue::full() const {
    return currentSize == CAPACITY;
}

std::size_t CompressionQueue::size() const {
    return currentSize;
}

} // namespace easycompress
