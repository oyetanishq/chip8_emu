cmake_minimum_required(VERSION 3.31)

# Specify that this is a C++ project
project(chip8_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to generate a compile_commands.json file
# attach this file in the .vscode folder
# "compileCommands": "${workspaceFolder}/build/compile_commands.json"
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create the main Executable Target
add_executable(${PROJECT_NAME} src/main.cpp)

find_package(SDL2 REQUIRED)
add_library(chip8 src/chip8.cpp src/instructions.cpp)

include_directories(include)
include_directories(${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE chip8)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
