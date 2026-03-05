#include <chip8.hpp>
#include <iostream>

// Fallback for unmapped opcodes
void Emulator::Chip8::OP_NULL() {
    std::cerr << "Unknown opcode: " << std::hex << this->opcode << std::endl;
}

// Group 0x0: Handles 0x00E0 (Clear screen) and 0x00EE (Return)
void Emulator::Chip8::OP_00E_() {
    // fetch last two nibbles
    uint8_t last_byte = this->opcode & 0x00FF;

    switch (last_byte) {
        case 0xE0: // 0x00E0: Clear the display
            memset(this->gfx, 0, sizeof(this->gfx));
            this->drawFlag = true;
            break;

        case 0xEE: // 0x00EE: Return from a subroutine
            this->sp--;
            this->pc = this->stack[this->sp];
            break;

        default:
            std::cerr << "Unknown 0x0 opcode: " << std::hex << this->opcode << std::endl;
            break;
    }
}

// 1nnn: Jump to location nnn
void Emulator::Chip8::OP_1nnn() {
    uint16_t address = this->opcode & 0x0FFF;
    this->pc = address;
}

// 2nnn: Call subroutine at location nnn
void Emulator::Chip8::OP_2nnn() {
    this->stack[this->sp] = this->pc;
    ++(this->sp);
    this->pc = this->opcode & 0x0FFF;
}

// 3xnn: Skip next instruction if Vx == nn
void Emulator::Chip8::OP_3xnn() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t byte = this->opcode & 0x00FF;

    if (this->v_reg[Vx] == byte)
        this->pc += 2;
}

// 4xnn: Skip next instruction if Vx != nn
void Emulator::Chip8::OP_4xnn() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t byte = this->opcode & 0x00FF;

    if (this->v_reg[Vx] != byte)
        this->pc += 2;
}

// 5xy0: Skip next instruction if Vx == Vy
void Emulator::Chip8::OP_5xy0() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t Vy = (this->opcode & 0x00F0) >> 4;

    if (this->v_reg[Vx] == this->v_reg[Vy])
        this->pc += 2;
}

// 6xnn: Set Vx = nn
void Emulator::Chip8::OP_6xnn() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t byte = this->opcode & 0x00FF;

    this->v_reg[Vx] = byte;
}

// 7xnn: Set Vx = Vx + nn
void Emulator::Chip8::OP_7xnn() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t byte = this->opcode & 0x00FF;

    this->v_reg[Vx] += byte;
}

// Group 0x8xy_: Bitwise operations and arithmetic
void Emulator::Chip8::OP_8xy_() {
    // Extract the X and Y register indices once to keep cases clean
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t Vy = (this->opcode & 0x00F0) >> 4;

    // Mask out everything except the last nibble
    uint8_t last_nibble = this->opcode & 0x000F;

    switch (last_nibble) {
        case 0x0: // 8xy0: Set Vx = Vy
            this->v_reg[Vx] = this->v_reg[Vy];
            break;
        
        case 0x1: // 8xy1: Set VX to (VX OR VY)
            this->v_reg[Vx] |= this->v_reg[Vy];
            break;

        case 0x2: // 8xy2: Set Vx = Vx AND Vy
            this->v_reg[Vx] &= this->v_reg[Vy];
            break;

        case 0x3: // 8xy3: Set VX to (VX XOR VY)
            this->v_reg[Vx] ^= this->v_reg[Vy];
            break;

        case 0x4: // 8xy4: Set Vx = Vx + Vy, set VF = carry
        {
            uint16_t sum = this->v_reg[Vx] + this->v_reg[Vy];
            this->v_reg[0xF] = (sum > 255) ? 1 : 0; // Carry flag
            this->v_reg[Vx] = sum & 0xFF;           // Keep lowest 8 bits
            break;
        }

        case 0x5: // 8xy5: Set Vx = Vx - Vy, set VF = NOT borrow.
        {
            // If Vx > Vy, there is no borrow, so VF is 1. Otherwise, 0.
            // We store the flag in a temporary variable before performing the math. 
            // This prevents an edge-case bug if Vx happens to be V[0xF].
            uint8_t not_borrow = (this->v_reg[Vx] >= this->v_reg[Vy]) ? 1 : 0;
            
            this->v_reg[Vx] -= this->v_reg[Vy];
            this->v_reg[0xF] = not_borrow;
            break;
        }

        case 0x6: // 8xy6: Shift Vx right by 1, set VF = LSB of Vx before shift.
        {
            // Extract the Least Significant Bit (LSB)
            uint8_t lsb = this->v_reg[Vx] & 0x1;
            
            this->v_reg[Vx] >>= 1;
            this->v_reg[0xF] = lsb;
            break;
        }

        case 0x7: // 8xy7: Set Vx = Vy - Vx, set VF = NOT borrow.
        {
            // If Vy > Vx, there is no borrow, so VF is 1. Otherwise, 0.
            uint8_t not_borrow = (this->v_reg[Vy] >= this->v_reg[Vx]) ? 1 : 0;
            
            this->v_reg[Vx] = this->v_reg[Vy] - this->v_reg[Vx];
            this->v_reg[0xF] = not_borrow;
            break;
        }

        case 0xE: // 8xyE: Shift Vx left by 1, set VF = MSB of Vx before shift.
        {
            // Extract the Most Significant Bit (MSB). 0x80 is 10000000 in binary.
            // Shift it right by 7 to make it a clean 0 or 1 for the VF register.
            uint8_t msb = (this->v_reg[Vx] >> 7) & 1;
            
            this->v_reg[Vx] <<= 1;
            this->v_reg[0xF] = msb;
            break;
        }

        

        default:
            std::cerr << "Unknown 0x8 opcode: " << std::hex << this->opcode << std::endl;
            break;
    }
}

// 9xy0: Skip next instruction if Vx != Vy
void Emulator::Chip8::OP_9xy0() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t Vy = (this->opcode & 0x00F0) >> 4;

    if (this->v_reg[Vx] != this->v_reg[Vy])
        this->pc += 2;
}

// Annn: Set I = nnn
void Emulator::Chip8::OP_Annn() {
    uint16_t address = this->opcode & 0x0FFF;
    this->i_reg = address;
}

// Bnnn: Jump to location nnn + V0
void Emulator::Chip8::OP_Bnnn() {
    uint16_t address = this->opcode & 0x0FFF;
    this->pc = address + this->v_reg[0];
}

// Cxnn: Set Vx = random byte AND nn
void Emulator::Chip8::OP_Cxnn() {
    uint16_t Vx = this->opcode & 0x0F00;
    this->v_reg[Vx] = (rand() % (0xFF + 1)) & (opcode & 0x00FF);
}

// Dxyn: Draw n-byte sprite at coordinates (Vx, Vy)
// which is a rectange of (8xN)
// 8 pixels width, and N pixels height
void Emulator::Chip8::OP_Dxyn() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t Vy = (this->opcode & 0x00F0) >> 4;
    uint8_t height = this->opcode & 0x000F;

    // The starting coordinates wrap around the screen if they are larger than the display
    // Assuming standard CHIP-8 resolution: 64x32
    uint8_t startX = this->v_reg[Vx] % 64;
    uint8_t startY = this->v_reg[Vy] % 32;

    // Reset the collision flag (VF) to 0 before drawing
    this->v_reg[0xF] = 0;

    // Loop through each row of the sprite
    for (uint8_t row = 0; row < height; ++row) {
        // Fetch the 8-bit row of the sprite from memory starting at register I
        uint8_t spriteByte = this->memory[this->i_reg + row];

        // Loop through each of the 8 pixels (bits) in this sprite row
        for (uint8_t col = 0; col < 8; ++col) {
            
            // Extract the current bit (pixel) from the sprite byte
            // 0x80 is 10000000 in binary. Shifting it right extracts each bit from left to right.
            uint8_t spritePixel = spriteByte & (0x80 >> col);

            // If the sprite pixel is ON (1), we need to draw it
            if (spritePixel) {
                // Standard CHIP-8 clips sprites that go off the edges of the screen
                if (startX + col >= 64) break; 
                if (startY + row >= 32) break;

                // Calculate the 1D index for the screen buffer
                uint16_t pixelIdx = (startY + row) * 64 + (startX + col);

                // If the pixel on the screen is already ON, a collision occurs. Set VF to 1.
                if (this->gfx[pixelIdx] == 1) {
                    this->v_reg[0xF] = 1;
                }

                // XOR the screen pixel with 1 (toggles it)
                this->gfx[pixelIdx] ^= 1;
            }
        }
    }

    this->drawFlag = true;
}

// Ex__: Keyboard instructions (Skip if key in Vx is pressed/not pressed)
void Emulator::Chip8::OP_Ex__() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t key = this->v_reg[Vx];        // The key we are checking (0-F)
    uint8_t action = this->opcode & 0x00FF; // The last byte tells us which check to perform

    switch (action) {
        case 0x9E: // Ex9E: Skip next instruction if key with the value of Vx is pressed.
            if (this->key[key] != 0)
                this->pc += 2;
            
            break;

        case 0xA1: // ExA1: Skip next instruction if key with the value of Vx is NOT pressed.
            if (this->key[key] == 0)
                this->pc += 2;

            break;

        default:
            std::cerr << "Unknown 0xE opcode: " << std::hex << this->opcode << std::endl;
            break;
    }
}

// Fx__: Timers, I/O, and memory operations (Delay, Sound, BCD, Reg Dump/Load)
void Emulator::Chip8::OP_Fx__() {
    uint8_t Vx = (this->opcode & 0x0F00) >> 8;
    uint8_t action = this->opcode & 0x00FF; // The last byte determines the specific operation

    switch (action) {
        case 0x07: // Fx07: Set Vx = delay timer value.
            this->v_reg[Vx] = this->delay_timer;
            break;

        case 0x0A: // Fx0A: Wait for a key press, store the value of the key in Vx.
        {
            bool keyPressed = false;

            // Check all 16 keys
            for (int i = 0; i < 16; ++i) {
                if (this->key[i] != 0) {
                    this->v_reg[Vx] = i;
                    keyPressed = true;
                    break; // Stop looking once we find a pressed key
                }
            }

            // If no key was pressed, decrement the PC by 2.
            // This forces the emulator to repeat this exact same instruction next cycle, 
            // effectively "blocking" execution until a key is pressed.
            if (!keyPressed)
                this->pc -= 2;

            break;
        }

        case 0x15: // Fx15: Set delay timer = Vx.
            this->delay_timer = this->v_reg[Vx];
            break;

        case 0x18: // Fx18: Set sound timer = Vx.
            this->sound_timer = this->v_reg[Vx];
            break;

        case 0x1E: // Fx1E: Set I = I + Vx.
            this->i_reg += this->v_reg[Vx];
            break;

        case 0x29: // Fx29: Set I = location of sprite for digit Vx.
            // Characters 0-F (in hex) are represented by a 4x5 font.
            // Since each character is 5 bytes long, multiplying the digit by 5 
            // gives the memory offset from where the fontset is loaded.
            this->i_reg = (5 * this->v_reg[Vx]);
            break;

        case 0x33: // Fx33: Store BCD representation of Vx in memory locations I, I+1, and I+2.
        {
            uint8_t value = this->v_reg[Vx];

            // Hundreds digit
            this->memory[this->i_reg + 0] = value / 100;
            // Tens digit
            this->memory[this->i_reg + 1] = (value / 10) % 10;
            // Ones digit
            this->memory[this->i_reg + 2] = value % 10;
            break;
        }

        case 0x55: // Fx55: Store registers V0 through Vx in memory starting at location I.
            for (uint8_t i = 0; i <= Vx; ++i)
                this->memory[this->i_reg + i] = this->v_reg[i];
        
            break;

        case 0x65: // Fx65: Read registers V0 through Vx from memory starting at location I.
            for (uint8_t i = 0; i <= Vx; ++i)
                this->v_reg[i] = this->memory[this->i_reg + i];

            break;

        default:
            std::cerr << "Unknown 0xF opcode: " << std::hex << this->opcode << std::endl;
            break;
    }
}
