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
        mCodes.push_back(0x0080016f);
    }
    if constexpr (config::TEST_CPU_EXECUTION) {

        CPU c(1 << 20);

        c.load(mCodes);
        auto memref = c.getMemRef();
        c.run();
        dump_state(c);
    } else {

        auto r_ins = std::get<RType>(decode(0x00c58733));
        std::cout << r_ins;
    }
    return 0;
}
