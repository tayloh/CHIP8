# taylohs CHIP8 Emulator

**Implementation of ambiguous opcodes**:  
8XY6 8XYE (Shift): VX is NOT set to the value of VY (modern behavior) <br>
BNNN (Jump with offset) is implemented, not BXNN <br>
FX55 FX65 (Store / load memory): I is not altered (modern behavior) <br>

## Run
Has only been compiled using mingw-w64 gcc on Windows 11. <br>
Raylib was compiled for this platform (recompile if you are on another platform). <br>
Compiling and running on Windows should work. <br>

To compile and link: <br>
`gcc src/main.c src/chip8.c -o bin/chip8 -Iexternal/raylib/include -Lexternal/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm` <br>


To run: `./bin/chip8.exe path_to_your_rom_file.ch8 <clock frequency>` <br>

Alternatively, you can try to run the precompiled binary in /bin.

## ROMs
Link to ROM files: https://github.com/loktar00/chip8/tree/master/roms