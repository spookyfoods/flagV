#pragma once
// objcopy gives you a flat byte stream. Your load() wants words. So the only
// real work here is reassembling bytes into little-endian uint32_t.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace loader {

    // Read a flat binary and reassemble it into 32-bit words.
    //
    // RISC-V is little-endian: the byte at the lowest address is the least
    // significant byte of the word. So bytes {0x93, 0x01, 0x00, 0x02} become
    // 0x02000193, which is what your decoder expects to see.
    inline std::vector<uint32_t> read_words(const std::string& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) {
            throw std::runtime_error("loader: cannot open '" + path + "'");
        }

        const std::streamsize size = f.tellg();
        if (size <= 0) {
            throw std::runtime_error("loader: '" + path + "' is empty");
        }
        f.seekg(0, std::ios::beg);

        std::vector<uint8_t> bytes(static_cast<size_t>(size));
        if (!f.read(reinterpret_cast<char*>(bytes.data()), size)) {
            throw std::runtime_error("loader: short read on '" + path + "'");
        }

        if (bytes.size() % 4 != 0) {
            // Shouldn't happen for pure .text, but catch it rather than
            // silently dropping a partial instruction.
            throw std::runtime_error("loader: '" + path + "' is " +
                                     std::to_string(bytes.size()) +
                                     " bytes, not a multiple of 4");
        }

        std::vector<uint32_t> words(bytes.size() / 4);
        for (size_t i = 0; i < words.size(); ++i) {
            words[i] = static_cast<uint32_t>(bytes[4 * i]) |
                       static_cast<uint32_t>(bytes[4 * i + 1]) << 8 |
                       static_cast<uint32_t>(bytes[4 * i + 2]) << 16 |
                       static_cast<uint32_t>(bytes[4 * i + 3]) << 24;
        }

        std::fprintf(stderr, "loader: %zu instructions from '%s'\n",
                     words.size(), path.c_str());
        return words;
    }

} // namespace loader
