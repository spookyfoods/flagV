#include "loader.h"
#include "system.h"
#include <iostream>

int main(int argc, char* argv[]) {
  // Default to 'fact.bin' if no file is provided as an argument
  std::string binary_path = (argc > 1) ? argv[1] : "fact.bin";

  try {
    std::cout << "[+] Loading binary file: " << binary_path << std::endl;

    // Use loader.h to read the file into 32-bit words
    std::vector<uint32_t> instructions = loader::read_words(binary_path);

    std::cout << "\n[+] Successfully loaded " << instructions.size()
              << " words.\n";
    std::cout << "-------------------------------------------\n";
    std::cout << " PC Index | Raw Hex   | Binary (32-bit)\n";
    std::cout << "-------------------------------------------\n";

    CPU c(1 << 24);
    c.load(instructions, 0x80000000);
    c.run();
  } catch (const std::exception& e) {
    std::cerr << "[-] Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
