#pragma once

#include <cstdint>

namespace Emulator {
    class Chip8 {
        private:
            uint16_t stack[16];                 // Stack
            uint16_t sp;                        // Stack pointer

            uint8_t memory[4096];               // Memory (4 kilo Byte)
            uint8_t v_reg[16];                  // V registers (V0-VF)

            uint16_t pc;                        // Program counter
            uint16_t opcode;                    // Current op code
            uint16_t i_reg;                     // Index register

            uint8_t delay_timer;                // Delay timer
            uint8_t sound_timer;                // Sound timer

            virtual void init() = 0;

        public:
            uint8_t  gfx[64 * 32];              // Graphics buffer
            uint8_t  key[16];                   // Keypad
            bool drawFlag;                      // Indicates a draw has occurred

            Chip8() {};
            virtual ~Chip8() {};

            virtual void emulate_cycle() = 0;               // Emulate one cycle
            virtual bool load_rom(const std::string& file_path) = 0;   // Load application
    };
}
