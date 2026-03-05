#include <chip8.hpp>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>

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

void Emulator::Chip8::setup_tables() {
    // Fill the table with a fallback first to prevent crash on bad opcodes
    for (int i = 0; i < 16; i++)
        main_table[i] = &Chip8::OP_NULL;

    // Map the first nibble (0x0 to 0xF) to the correct function
    main_table[0x0] = &Chip8::OP_00E_;
    main_table[0x1] = &Chip8::OP_1nnn;
    main_table[0x2] = &Chip8::OP_2nnn;
    main_table[0x3] = &Chip8::OP_3xnn;
    main_table[0x4] = &Chip8::OP_4xnn;
    main_table[0x5] = &Chip8::OP_5xy0;
    main_table[0x6] = &Chip8::OP_6xnn;
    main_table[0x7] = &Chip8::OP_7xnn;
    main_table[0x8] = &Chip8::OP_8xy_;
    main_table[0x9] = &Chip8::OP_9xy0;
    main_table[0xA] = &Chip8::OP_Annn;
    main_table[0xB] = &Chip8::OP_Bnnn;
    main_table[0xC] = &Chip8::OP_Cxnn;
    main_table[0xD] = &Chip8::OP_Dxyn;
    main_table[0xE] = &Chip8::OP_Ex__;
    main_table[0xF] = &Chip8::OP_Fx__;
}

Emulator::Chip8::Chip8() {
    setup_tables();
}
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

    srand(time(NULL));
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

    std::cout << "Loaded Succesfully into ROM: " << file_path << std::endl;

    rom_file.close();
    return true;
}

bool Emulator::Chip8::load_rom_from_buffer(const std::vector<uint8_t>& buffer) {
    this->init();

    std::cout << "Loading ROM from buffer" << std::endl;

    if (buffer.size() > (4096 - ROM_START)) {
        std::cerr << "Error: ROM file is too large to fit in CHIP-8 memory!\n";
        return false;
    }
    
    std::copy(buffer.begin(), buffer.end(), &this->memory[ROM_START]);
    return true;
}

void Emulator::Chip8::emulate_cycle() {
    // opcode is of 2 byte, but memory stores 2 byte per pc,
    // so left shift 8 for pc, and overlap with pc+1
    // 
    // example: 
    // memory[pc+0] == 0xA2 << 8  -> (10100010 00000000)
    // memory[pc+1] == 0xF0       -> (         11110000)
    // opcode       == 0xA2F0     -> (10100010 11110000)
    //
    uint16_t higher_byte = this->memory[this->pc + 0];
    uint16_t lower_byte  = this->memory[this->pc + 1];
    this->opcode = (higher_byte << 8) | lower_byte;

    // increment "program counter" before execution
    this->pc += 2;

    // extract the first nibble: 0xABCD -> 0xA,
    // and execute the pc opcode from the array of functions
    uint8_t first_nibble = (this->opcode & 0xF000) >> 12;
    (this->*main_table[first_nibble])();

    // update timers: delay, and sound
    if (this->delay_timer > 0)
        --(this->delay_timer);
    
    if (this->sound_timer > 0) {
        if(this->sound_timer == 1) {
            // TODO: BEEP
        }
        
        --(this->sound_timer);
    }
}
