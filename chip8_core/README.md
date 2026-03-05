# Refered Docs

1. https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter
2. https://austinmorlan.com/posts/chip8_emulator
3. https://github.com/JamesGriffin/CHIP-8-Emulator
4. https://github.com/aquova/chip8-book/releases/download/v1.1/chip8.pdf
5. https://dominikrys.com/posts/compiling-chip8-to-wasm

# build steps
1. for chip8_core build
    ```bash
    # for chip8_core build
    mkdir -p build/core
    cd build/core
    
    cmake ../..
    make
    ```

2. for wasm binary build
    ```bash
    # for chip8_wasm build
    mkdir -p build/wasm
    cd build/wasm

    emcmake cmake ../..
    make
    ```