#include <iostream>
#include <cstring>
#include <chip8.hpp>
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>

// Keypad keymap
uint8_t keymap[16] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v,
};

signed main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: ./chip8_core <ROM file_path>" << std::endl;
        return 1;
    }
    std::string file_path = argv[1];

    int width  = 1024;                   // Window width
    int height = 512;                    // Window height

    SDL_Window* window = NULL;                   // screen
    Emulator::Chip8 chip8 = Emulator::Chip8();   // chip8 emulator

    // Initialize SDL
    if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Create window
    window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width, height, SDL_WINDOW_SHOWN
    );
    if (window == NULL){
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 2;
    }

    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, width, height);

    // Create texture that stores frame buffer
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    // Temporary pixel buffer
    uint32_t pixels[2048];
    
    load_rom_file:
    if (!chip8.load_rom(argv[1]))
        return 2;

    // Emulation loop
    while (true) {
        chip8.emulate_cycle();

        // Process SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);

            // Process keydown events
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    exit(0);

                if (e.key.keysym.sym == SDLK_F1)
                    goto load_rom_file;      // *gasp*, a goto statement!
                                             // Used to reset/reload ROM

                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keymap[i]) {
                        chip8.key[i] = 1;
                    }
                }
            }
            // Process keyup events
            if (e.type == SDL_KEYUP) {
                for (int i = 0; i < 16; ++i) {
                    if (e.key.keysym.sym == keymap[i]) {
                        chip8.key[i] = 0;
                    }
                }
            }
        }

        // If draw occurred, redraw SDL screen
        if (chip8.drawFlag) {
            chip8.drawFlag = false;

            // Store pixels in temporary buffer
            for (int i = 0; i < 2048; ++i) {
                uint8_t pixel = chip8.gfx[i];
                pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
            }
            // Update SDL texture
            SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(Uint32));
            // Clear screen and render
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        // Sleep to slow down emulation speed
        std::this_thread::sleep_for(std::chrono::microseconds(1200));
    }

    return 0;
}
