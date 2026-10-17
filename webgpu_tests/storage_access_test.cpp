#include "spirv_preprocess.h"

#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    // Storage-buffer access qualifiers change; image and unrelated members stay.
    const uint32_t words[] = {
        0x07230203, 0x00010300, 0, 101, 0,
        (3u << 16) | 71, 20, 25,
        (4u << 16) | 72, 10, 0, 25,
        (3u << 16) | 71, 30, 25,
        (4u << 16) | 72, 11, 0, 25,
        (3u << 16) | 71, 20, 24,
        (4u << 16) | 32, 100, 12, 10,
        (4u << 16) | 59, 100, 20, 12,
    };
    Vector<uint8_t> bytes;
    bytes.resize(sizeof(words));
    std::memcpy(bytes.ptrw(), words, sizeof(words));
    auto result = spirv_preprocess::normalize_storage_buffer_access(bytes);
    assert(result.size() == sizeof(words) - 7 * sizeof(uint32_t));
    const auto* output = reinterpret_cast<const uint32_t*>(result.ptr());
    assert(output[6] == 30 && output[7] == 25); // Image NonReadable retained.
    assert(output[9] == 11 && output[11] == 25); // Unrelated member retained.
    assert(output[13] == 20 && output[14] == 24); // NonWritable retained.
    auto repeated = spirv_preprocess::normalize_storage_buffer_access(result);
    assert(repeated.size() == result.size());
    assert(std::memcmp(repeated.ptr(), result.ptr(), result.size()) == 0);
    bytes.resize(21);
    assert(spirv_preprocess::normalize_storage_buffer_access(bytes).size() == 21);
    std::puts("PASS: storage access normalization preserves image/read-only qualifiers and malformed input");
}
