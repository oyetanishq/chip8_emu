#include <chip8.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(chip8_module) {
    // Expose std::vector<uint8_t> so JS can pass byte arrays to C++
    register_vector<uint8_t>("VectorUint8");

    class_<Emulator::Chip8>("Chip8")
        .constructor<>()
        .function("emulate_cycle", &Emulator::Chip8::emulate_cycle)
        .function("load_rom_from_buffer", &Emulator::Chip8::load_rom_from_buffer)
        .property("drawFlag", &Emulator::Chip8::drawFlag)
        
        // Return a direct Uint8Array view of the graphics memory (64 * 32 = 2048 pixels)
        .function("getGfx", optional_override([](Emulator::Chip8& self) {
            return val(typed_memory_view(2048, self.gfx));
        }))
        // Return a direct Uint8Array view of the 16 keys
        .function("getKeys", optional_override([](Emulator::Chip8& self) {
            return val(typed_memory_view(16, self.key));
        }));
}
