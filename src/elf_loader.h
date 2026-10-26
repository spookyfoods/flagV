#ifndef ELF_LOADER_H
#define ELF_LOADER_H
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

struct ElfError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ElfHeader {
    uint32_t entry;     // Entry point of the program, the loader utilizes it
    uint32_t phoff;     // file offset of Program Header
    uint16_t phentsize; // Size of one entry in the Program header, all entries
                        // are same size
    uint16_t phnum;     // Number of entries in the Program Header, phnum *
                        // phentsize = total size of ph
    uint32_t flags; // Holds processor-specific flags, defined as macros of form
                    // EF_'machine_flag'
};

// Throws ElfError with a specific message for each problem.
ElfHeader parse_header(std::span<const uint8_t> file);
std::vector<uint8_t> read_file(const std::string& path);
#endif // ELF_LOADER_H
