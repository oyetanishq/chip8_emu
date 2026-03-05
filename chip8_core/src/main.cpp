#include <iostream>
#include <cstring>
#include <chip8.hpp>

signed main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: ./chip8_core <ROM file_path>" << std::endl;
        return 1;
    }
    std::string file_path = argv[1];

    Emulator::Chip8 chip8 = Emulator::Chip8();
    
    if (!chip8.load_rom(argv[1]))
        return 2;

    return 0;
}
