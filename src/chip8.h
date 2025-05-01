#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <assert.h>

enum
{
    CHIP8_MEMORY_SIZE = 4096,
    CHIP8_STACK_SIZE = 64,
    CHIP8_NUM_VAR_REGISTERS = 16,
    CHIP8_SCREEN_WIDTH = 64,
    CHIP8_SCREEN_HEIGHT = 32
};

typedef struct
{
    uint16_t arr[CHIP8_STACK_SIZE];
    uint8_t top;
} Stack;

typedef struct
{
    // Main memory 4 kB
    uint8_t memory[CHIP8_MEMORY_SIZE];

    // General-purpose variable registers numbered 0 through F hexadecimal
    uint8_t V[CHIP8_NUM_VAR_REGISTERS];

    // Program counter, points at current instruction in memory
    uint16_t pc;

    // Index register, for pointing at locations in memory
    uint16_t I;

    // Stack
    Stack stack;

    // Delay and sound timers
    uint8_t delay_timer;
    uint8_t sound_timer;

    // Display
    uint8_t display[CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT];

    // Keypad
    uint8_t keypad[16];

} Chip8;

// Chip8
void chip8_init(Chip8 *chip8);
int chip8_load_rom(Chip8 *chip8, const char *filename);
void chip8_cycle(Chip8 *chip8);
void chip8_execute_opcode(Chip8 *chip8, uint16_t opcode);
void chip8_update_timers(Chip8 *chip8);

// Stack
void stack_init(Stack *stack);
void stack_push(Stack *stack, uint16_t value);
uint16_t stack_pop(Stack *stack);

#endif