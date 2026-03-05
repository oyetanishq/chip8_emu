/// <reference types="emscripten" />

// Wrapper for std::vector<uint8_t> exposed via register_vector.
export interface VectorUint8 {
    push_back(value: number): void;
    resize(size: number, value: number): void;
    size(): number;
    get(index: number): number;
    set(index: number, value: number): boolean;
    delete(): void;
}

// Wrapper for the Emulator::Chip8 C++ class.
export interface Chip8 {
    drawFlag: boolean;

    emulate_cycle(): void;
    load_rom_from_buffer(buffer: VectorUint8): void;

    getGfx(): Uint8Array;
    getKeys(): Uint8Array;

    delete(): void;
}

/**
 * The instantiated WebAssembly module containing your bindings
 * and Emscripten's memory heap views.
 */
export interface Chip8Module {
    // Classes exposed via embind
    Chip8: {
        new (): Chip8;
    };
    VectorUint8: {
        new (): VectorUint8;
    };

    // Emscripten memory views (crucial for reading getGfxPtr/getKeyPtr)
    HEAP8: Int8Array;
    HEAP16: Int16Array;
    HEAP32: Int32Array;
    HEAPU8: Uint8Array;
    HEAPU16: Uint16Array;
    HEAPU32: Uint32Array;
    HEAPF32: Float32Array;
    HEAPF64: Float64Array;

    _malloc(size: number): number;
    _free(ptr: number): void;
}

/**
 * Because we used -s MODULARIZE=1 and -s EXPORT_ES6=1,
 * the file exports a default async factory function.
 */
export default function Module(moduleOverrides?: Partial<Chip8Module>): Promise<Chip8Module>;
