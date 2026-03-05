#include <chip8.hpp>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <fstream>

const uint16_t ROM_START = 0x200;

const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Emulator::Chip8::Chip8() {}
Emulator::Chip8::~Chip8() {}

void Emulator::Chip8::init() {
    this->pc      = ROM_START;   // set program counter to ROM_START
    this->opcode  = 0;           // reset op code
    this->i_reg   = 0;           // reset i_reg
    this->sp      = 0;           // reset stack pointer

    // clear: display, and key
    memset(this->gfx, 0, sizeof(this->gfx));
    memset(this->key, 0, sizeof(this->key));

    // clear: stack, memory, and v_registors
    memset(this->stack, 0, sizeof(this->stack));   // clear the display
    memset(this->memory, 0, sizeof(this->memory)); // clear the display
    memset(this->v_reg, 0, sizeof(this->v_reg));   // clear the display

    // load chip8's fontset in memory
    for (int i=0; i<80; i++)
        this->memory[i] = chip8_fontset[i];

    // reset timers
    this->delay_timer = 0;
    this->sound_timer = 0;
}

bool Emulator::Chip8::load_rom(const std::string& file_path) {
    this->init();

    std::cout << "Loading ROM: " << file_path << std::endl;

    // open the file as a binary stream.
    std::ifstream rom_file(file_path, std::ios::binary);

    if (!rom_file.is_open()) {
        std::cerr << "Error: Could not open ROM file: " << file_path << "\n";
        return false;
    }

    // move the cursor at the end of the file,
    // then tellg() gives us the exact byte size.
    rom_file.seekg(0, std::ios::end);
    std::streampos size = rom_file.tellg();

    // CHIP-8 has 4096 bytes total, but the first 512 (0x200) are reserved.
    if (size > (4096 - ROM_START)) {
        std::cerr << "Error: ROM file is too large to fit in CHIP-8 memory!\n";
        return false;
    }

    // move the cursor at the start, to copy the ROM in the Memory.
    rom_file.seekg(0, std::ios::beg);
    rom_file.read((char*)(&memory[ROM_START]), size);

    rom_file.close();
    return true;
}
