#include "config.h"
#include "system.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
int main() {

    std::vector<uint32_t> mCodes{};

    if constexpr (config::LOAD_OBJDUMP_FILE) {
        std::ifstream inputFile("objdump.txt");
        std::string line;

        if (!inputFile.is_open()) {
            std::cerr << "Failed to open objdump.txt\n";
            return 1;
        }

        while (std::getline(inputFile, line)) {
            if (!line.empty() && line.find("<") != std::string::npos) {
                break;
            }
        }

        while (std::getline(inputFile, line)) {
            std::string addressToken;
            std::string hexToken;
            std::stringstream ss(line);

            if (ss >> addressToken >> hexToken) {

                if (!addressToken.empty() && addressToken.back() == ':') {
                    try {
                        mCodes.push_back(static_cast<uint32_t>(
                            std::stoul(hexToken, nullptr, 16)));
                    } catch (const std::invalid_argument&) {
                        continue;
                    } catch (const std::out_of_range&) {
                        continue;
                    }
                }
            }
        }
    } else {
        mCodes.push_back(
            0x00500293); // 1.  addi x5, x0, 5     (I-type) -> x5 = 5
        mCodes.push_back(
            0x00028333); // 2.  add  x6, x5, x0     (R-type) -> x6 = 5
        mCodes.push_back(
            0x00A00313); // 3.  addi x6, x0, 10    (I-type) -> x6 = 10
        mCodes.push_back(
            0x006283b3); // 4.  add  x7, x5, x6     (R-type) -> x7 = 15
        mCodes.push_back(
            0x0012c433); // 5.  xor  x8, x5, x1     (R-type) -> x8 = 5
        mCodes.push_back(
            0x002004b3); // 6.  sll  x9, x0, x2     (R-type) -> x9 = 0
        mCodes.push_back(
            0x00328533); // 7.  add  x10, x5, x3    (R-type) -> x10 = 5
        mCodes.push_back(
            0x002285b3); // 8.  add  x11, x5, x2    (R-type) -> x11 = 5
        mCodes.push_back(
            0x00001637); // 9.  lui  x12, 1        (U-type) -> x12 = 4096
        mCodes.push_back(
            0x00C60693); // 10. addi x13, x12, 12  (I-type) -> x13 = 4108
    }
    if constexpr (config::TEST_CPU_EXECUTION) {

        CPU c(1 << 20);

        c.load(mCodes);
        auto& memref = c.getMemRef();
        c.run();
        dump_state(c);
    }

    CPU cpu(1 << 10);
    auto& mem = cpu.getMemRef();
    mem.write32(8, 0, 0x12'34'56'78);
    // lb x2, 9(x1)
    int32_t trash{};
    cpu.execute_context(decode(0x00908103), trash);
    // r2 should have the value 0x56

    auto r_ins = std::get<RType>(decode(0x00c58733));
    std::cout << r_ins;

    return 0;
}
