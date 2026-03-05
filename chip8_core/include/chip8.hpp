#pragma once

#include <cstdint>
#include <iostream>

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

            // function pointer type for opcode handlers
            // primary dispatch table for the first nibble (0x0 to 0xF)
            typedef void (Chip8::*OpcodeHandler)();
            OpcodeHandler main_table[16];

            void init();
            void setup_tables();

            // --- Instruction Handlers ---
            void OP_00E_();      // System instructions: 00E0 (Clear screen) and 00EE (Return)
            void OP_1nnn();      // Jump to location nnn
            void OP_2nnn();      // Call subroutine at location nnn
            void OP_3xnn();      // Skip next instruction if Vx == nn
            void OP_4xnn();      // Skip next instruction if Vx != nn
            void OP_5xy0();      // Skip next instruction if Vx == Vy
            void OP_6xnn();      // Set register Vx = nn
            void OP_7xnn();      // Add nn to register Vx (Vx = Vx + nn)
            void OP_8xy_();      // Arithmetic and logical operations (Set, OR, AND, XOR, ADD, SUB, Shift)
            void OP_9xy0();      // Skip next instruction if Vx != Vy
            void OP_Annn();      // Set I = nnn
            void OP_Bnnn();      // Jump to location nnn + V0
            void OP_Cxnn();      // Set Vx = random byte AND nn
            void OP_Dxyn();      // Draw n-byte sprite at coordinates (Vx, Vy)
            void OP_Ex__();      // Keyboard instructions (Skip if key in Vx is pressed/not pressed)
            void OP_Fx__();      // Timers, I/O, and memory operations (Delay, Sound, BCD, Reg Dump/Load)
            void OP_NULL();      // Fallback for unhandled/empty opcodes

        public:
            uint8_t  gfx[64 * 32];              // Graphics buffer
            uint8_t  key[16];                   // Keypad
            bool drawFlag;                      // Indicates a draw has occurred

            Chip8();
            ~Chip8();

            void emulate_cycle();               // Emulate one cycle
            bool load_rom(const std::string& file_path);   // Load application
    };
}
