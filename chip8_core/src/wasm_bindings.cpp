#include <chip8.hpp>
#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(chip8_module) {
    // Expose std::vector<uint8_t> so JS can pass byte arrays to C++
    register_vector<uint8_t>("VectorUint8");

    class_<Emulator::Chip8>("Chip8")
        .constructor<>()
        .function("emulate_cycle", &Emulator::Chip8::emulate_cycle)
        .function("load_rom_from_buffer", &Emulator::Chip8::load_rom_from_buffer)
        .property("drawFlag", &Emulator::Chip8::drawFlag)
        
        // Expose pointers to the arrays so JS can read/write directly to the WASM memory heap
        .function("getGfxPtr", optional_override([](Emulator::Chip8& self) {
            return reinterpret_cast<uintptr_t>(self.gfx);
        }))
        .function("getKeyPtr", optional_override([](Emulator::Chip8& self) {
            return reinterpret_cast<uintptr_t>(self.key);
        }));
}
