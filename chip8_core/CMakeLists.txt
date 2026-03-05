cmake_minimum_required(VERSION 3.31)

# Specify that this is a C++ project
project(chip8_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to generate a compile_commands.json file
# attach this file in the .vscode folder
# "compileCommands": "${workspaceFolder}/build/compile_commands.json"
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
add_library(chip8 src/chip8.cpp src/instructions.cpp)

if (EMSCRIPTEN)
    # --- WEB BUILD (WASM) ---
    message(STATUS "Configuring WebAssembly build via Emscripten...")

    # Create the WASM executable target
    add_executable(chip8_wasm src/wasm_bindings.cpp)
    target_link_libraries(chip8_wasm PRIVATE chip8)

    # Set Emscripten linker flags
    target_compile_options(chip8_wasm PRIVATE "-O3")

    set_target_properties(chip8_wasm PROPERTIES 
        LINK_FLAGS "-O3 -lembind -s MODULARIZE=1 -s EXPORT_ES6=1 -s SINGLE_FILE=1 -s ENVIRONMENT=web"
        OUTPUT_NAME "chip8_wasm" 
        SUFFIX ".js"
    )
else()
    # --- NATIVE BUILD (Desktop) ---
    message(STATUS "Configuring Native build...")

    # Create the main Executable Target
    add_executable(${PROJECT_NAME} src/main.cpp)

    find_package(SDL2 REQUIRED)

    include_directories(${SDL2_INCLUDE_DIRS})

    target_link_libraries(${PROJECT_NAME} PRIVATE chip8)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()
