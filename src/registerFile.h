#include <array>
#include <cstdint>
#include <iostream>

class RegisterFile {
  private:
    std::array<uint32_t, 32> regs{};

  public:
    constexpr uint32_t read(uint8_t index) const { return regs[index]; }

    constexpr void write(uint8_t index, uint32_t value) {
        if (index == 0) {
            return;
        }
        regs[index] = value;
    }

    constexpr uint32_t operator[](uint8_t index) const { return regs[index]; }
};
