#include <fstream>
#include <sstream>
#include <vector>
int main() {
    std::vector<uint32_t> mCodes{};
    std::ifstream inputFile("objdump.txt");
    std::string line;
    while (std::getline(inputFile, line)) {
        if (line.empty() || line.find("<") == std::string::npos) {
            continue;
        }
        break;
    }
    std::getline(inputFile, line);
    while (std::getline(inputFile, line)) {
        std::string hex;
        std::stringstream ss(line);
        ss >> hex;
        ss >> hex;
        mCodes.push_back(static_cast<uint32_t>(std::stoul(hex, nullptr, 16)));
    }
}
