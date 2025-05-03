#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

#include "chip8.h"

/// ********************
/// Chip8 functions    *
/// ********************

#define CHIP8_UNKNOWN_OPCODE "Error: Unknown instruction encountered."

static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void chip8_init(Chip8 *chip8)
{
    memset(chip8->memory, 0, sizeof(chip8->memory));
    memset(chip8->V, 0, sizeof(chip8->V));
    memset(chip8->display, 0, sizeof(chip8->display));
    memset(chip8->keypad, 0, sizeof(chip8->keypad));

    stack_init(&chip8->stack);

    // Load fontset into memory
    for (uint16_t i = 0; i < 80; i++)
    {
        // Load at 0x50
        chip8->memory[0x50 + i] = chip8_fontset[i];
    }

    chip8->I = 0;

    // Program starts at 0x200
    chip8->pc = 0x200;

    chip8->delay_timer = 0;
    chip8->sound_timer = 0;

    // Seed rand()
    srand(time(NULL));
}
int chip8_load_rom(Chip8 *chip8, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        return 1;
    }

    // To read into memory position 0x200
    // We grab the address of position 0x200 in our memory array
    // chip8->memory[0x200] and *(chip8->memory + 0x200) are equivalent
    // &chip8->memory[0x200] and chip8->memory + 0x200
    // void* in C is a generic pointer type (we can pass in *uint8_t)
    fread(&chip8->memory[0x200], 1, CHIP8_MEMORY_SIZE - 0x200, file);

    fclose(file);

    return 0;
}

void chip8_pass_input(Chip8 *chip8, uint8_t input[])
{
    for (uint8_t i = 0; i < CHIP8_NUM_KEYS; i++)
    {
        chip8->keypad[i] = input[i];
    }
}

void chip8_cycle(Chip8 *chip8)
{
    // fetch-decode-execute one cycle

    // RAM:
    //      Byte1 00000001
    //      Byte2 00010010

    // Instruction: Byte1 Byte2

    // uint16_t: 00000000 00000000
    // chip8->memory[chip8->pc] << 8  => 00000001 00000000
    // | chip8->memory[chip8->pc + 1] => 00000001 00010010

    // Instruction: 00000001 00010010

    // In code:
    // uint16_t op = 0;
    // op = chip8->memory[chip8->pc] << 8;
    // op = op | chip8->memory[chip8->pc + 1];

    uint16_t opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;

    chip8_execute_opcode(chip8, opcode);
}

void chip8_execute_opcode(Chip8 *chip8, uint16_t opcode)
{

    // Instruction
    // Nib1 Nib2 Nib3 Nib4
    // 0000 0000 0000 0000

    // X = Nib2, lookup VX
    // Y = Nib3, lookup VY
    // N = Nib4, a 4-bit number
    // NN = Nib3 Nib4 (second byte), and 8-bit number
    // NNN = Nib2 Nib3 Nib4, a 12-bit memory address

    // Decode instruction
    uint8_t instruction_category = (opcode >> 12) & 0xF;

    // Nib2
    uint8_t X = (opcode >> 8) & 0xF;

    // Nib3
    uint8_t Y = (opcode >> 4) & 0xF;

    // Nib4
    uint8_t N = opcode & 0xF;

    // Second byte (Nib3 Nib4)
    uint8_t NN = opcode & 0xFF;

    // Nib2 Nib3 Nib4
    uint16_t NNN = opcode & 0xFFF;

    // Execute instruction
    switch (instruction_category)
    {
    case 0x0:

        switch (opcode)
        {

        // 00E0 Clear screen
        case 0x00E0:
            memset(chip8->display, 0, sizeof(chip8->display));

            chip8_debug_printf(chip8, "00E0 Clear screen");
            break;

        // 00EE Return from subroutine
        case 0x00EE:
            chip8->pc = stack_pop(&chip8->stack);

            chip8_debug_printf(chip8, "00EE Return (pop stack) - PC set to %X", chip8->pc);
            break;

        default:
            printf(CHIP8_UNKNOWN_OPCODE);
            break;
        }

        break;

    // 1NNN Jump
    case 0x1:
        chip8->pc = NNN;

        chip8_debug_printf(chip8, "1NNN 1%X Jump - PC set to %X", NNN, NNN);
        break;

    // 2NNN Jump to subroutine
    case 0x2:
        stack_push(&chip8->stack, chip8->pc);
        chip8->pc = NNN;

        chip8_debug_printf(chip8, "2NNN 2%X Subroutine (push stack) then PC set to %X", NNN, NNN);
        break;

    // 3XNN Skip one instruction if VX == NN
    case 0x3:
        if (chip8->V[X] == NN)
        {
            chip8->pc += 2;
        }

        chip8_debug_printf(chip8, "3XNN 3%X%X Skip one instruction if VX == NN", X, NN);
        break;

    // 4XNN Skip one instruction if VX != NN
    case 0x4:
        if (chip8->V[X] != NN)
        {
            chip8->pc += 2;
        }

        chip8_debug_printf(chip8, "4XNN 4%X%X Skip one instruction if VX != NN", X, NN);
        break;

    // 5XY0 Skip one instruction if VX == VY
    case 0x5:
        if (chip8->V[X] == chip8->V[Y])
        {
            chip8->pc += 2;
        }

        chip8_debug_printf(chip8, "5XY0 5%X%X0 Skip one instruction if VX == VY", X, Y);
        break;

    // 6XNN Set
    case 0x6:
        chip8->V[X] = NN;

        chip8_debug_printf(chip8, "6XNN 6%X%X Set VX to NN", X, NN);
        break;

    // 7XNN Add
    case 0x7:
        // CHIP8 expects overflow to happen, don't treat it
        chip8->V[X] += NN;

        chip8_debug_printf(chip8, "7XNN 7%X%X Add NN to VX", X, NN);
        break;

    case 0x8:
        switch (N)
        {
        // 8XY0 Set
        case 0x0:
            chip8->V[X] = chip8->V[Y];

            chip8_debug_printf(chip8, "8XY0 8%X%X0 Set VX = VY", X, Y);
            break;

        // 8XY1 OR
        case 0x1:
            chip8->V[X] |= chip8->V[Y];

            chip8_debug_printf(chip8, "8XY1 8%X%X1 Set VX |= VY", X, Y);
            break;

        // 8XY2 AND
        case 0x2:
            chip8->V[X] &= chip8->V[Y];

            chip8_debug_printf(chip8, "8XY2 8%X%X2 Set VX &= VY", X, Y);
            break;

        // 8XY3 XOR
        case 0x3:
            chip8->V[X] ^= chip8->V[Y];

            chip8_debug_printf(chip8, "8XY3 8%X%X3 Set VX ^= VY", X, Y);
            break;

        // 8XY5 Add with carry
        case 0x4:
        {
            uint16_t sum = chip8->V[X] + chip8->V[Y];
            chip8->V[0xF] = (sum > 0xFF) ? 1 : 0;
            chip8->V[X] = sum & 0xFF;

            chip8_debug_printf(chip8, "8XY4 8%X%X4 Set VX = VX + VY with carry", X, Y);
            break;
        }

        // 8XY5 Subtract with borrow (VX - VY)
        case 0x5:
            chip8->V[0xF] = (chip8->V[X] >= chip8->V[Y]) ? 1 : 0;
            chip8->V[X] = chip8->V[X] - chip8->V[Y];

            chip8_debug_printf(chip8, "8XY5 8%X%X5 Set VX = VX - VY with borrow", X, Y);
            break;

        // 8XY6 Shift right
        case 0x6:
            chip8->V[0xF] = chip8->V[X] & 0x1;
            chip8->V[X] >>= 1;

            chip8_debug_printf(chip8, "8XY6 8%X%X6 Set VX >>= 1", X, Y);
            break;

        // 8XY7 Subtract with borrow (VY-VX)
        case 0x7:
            chip8->V[0xF] = (chip8->V[Y] >= chip8->V[X]) ? 1 : 0;
            chip8->V[X] = chip8->V[Y] - chip8->V[X];

            chip8_debug_printf(chip8, "8XY5 8%X%X5 Set VX = VY - VX with borrow", X, Y);
            break;

        // 8XY6 Shift left
        case 0xE:
            chip8->V[0xF] = (chip8->V[X] >> 7) & 1;
            chip8->V[X] <<= 1;

            chip8_debug_printf(chip8, "8XY6 8%X%X6 Set VX <<= 1", X, Y);
            break;

        default:
            printf(CHIP8_UNKNOWN_OPCODE);
            break;
        }

        break;

    // 9XY0 Skip one instruction if VX != VY
    case 0x9:
        if (chip8->V[X] != chip8->V[Y])
        {
            chip8->pc += 2;
        }

        chip8_debug_printf(chip8, "9XY0 9%X%X0 Skip one instruction if VX != VY", X, Y);
        break;

    // ANNN Set index
    case 0xA:
        chip8->I = NNN;

        chip8_debug_printf(chip8, "ANNN A%X Set I = NNN", NNN);
        break;

    // BNNN Jump to NNN + V0
    case 0xB:
        chip8->pc = NNN + chip8->V[0];

        chip8_debug_printf(chip8, "BNNN B%X Jump NNN+V0 - PC set to %X", NNN, NNN + chip8->V[0]);
        break;

    // CXNN Random
    case 0xC:
    {
        uint8_t random_number = (uint8_t)(rand() % 256);
        chip8->V[X] = random_number & NN;

        chip8_debug_printf(chip8, "CXNN C%X%X Random - VX randomized", X, NN);
        break;
    }

    // DXYN Draw
    case 0xD:
    {
        // x=64 should wrap to 0 as input
        // we can use binary AND to "filter" the width and height, instead of modulo (which is slower)
        // Example: 64 = 0b01000000, 64-1 = 0b00111111
        // Since width can only be between 0b00000000 and 0b0011111111, we can mask the input using bin of 63
        // This only works if width and height are powers of 2.
        uint8_t x_coord = chip8->V[X] & (CHIP8_SCREEN_WIDTH - 1);
        uint8_t y_coord = chip8->V[Y] & (CHIP8_SCREEN_HEIGHT - 1);

        chip8_debug_printf(chip8, "DXYN D%X%X%X Draw %X sprite rows drawn at (%X, %X) from memory location %X", X, Y, N, N, x_coord, y_coord, chip8->I);

        chip8->V[0xF] = 0;

        for (uint8_t i = 0; i < N; i++)
        {
            if (y_coord >= CHIP8_SCREEN_HEIGHT)
            {
                break;
            }

            uint16_t memory_location = chip8->I + i;
            uint8_t sprite_row = chip8->memory[memory_location];

            // Skip empty sprite rows
            if (sprite_row == 0)
            {
                y_coord++;
                continue;
            }

            uint8_t curr_x_coord = x_coord;

            // Current sprite row: 0b 0011 0010
            for (int8_t j = 7; j >= 0; j--)
            {
                if (curr_x_coord >= CHIP8_SCREEN_WIDTH)
                {
                    break;
                }

                uint8_t sprite_row_pixel = (sprite_row >> j) & 1;

                uint16_t linear_index = y_coord * CHIP8_SCREEN_WIDTH + curr_x_coord;
                uint8_t screen_pixel = chip8->display[linear_index];

                if (sprite_row_pixel)
                {
                    if (screen_pixel)
                    {
                        chip8->V[0xF] = 1;
                    }
                    // If sprite pixel is on, flip the screen pixel (using XOR with 1)
                    chip8->display[linear_index] ^= 1;
                }
                curr_x_coord++;
            }
            y_coord++;
        }

        break;
    }
    case 0xE:

        switch (NN)
        {
        // EX9E Skip if key pressed
        case 0x9E:
            if (chip8->keypad[chip8->V[X]] == 1)
            {
                chip8->pc += 2;
            }

            chip8_debug_printf(chip8, "EX9E E%X9E Skip instruction if key VX is pressed", X);
            break;

        // EXA1 Skip if key not pressed
        case 0xA1:
            if (chip8->keypad[chip8->V[X]] == 0)
            {
                chip8->pc += 2;
            }

            chip8_debug_printf(chip8, "EX9E E%X9E Skip instruction if key VX is not pressed", X);
            break;

        default:
            printf(CHIP8_UNKNOWN_OPCODE);
            break;
        }

        break;

    case 0xF:

        switch (NN)
        {
        // FX07 Set VX to delay timer value
        case 0x07:
            chip8->V[X] = chip8->delay_timer;

            chip8_debug_printf(chip8, "FX07 F%X07 Set VX to delay timer value", X);
            break;

        // FX15 Set delay timer
        case 0x15:
            chip8->delay_timer = chip8->V[X];

            chip8_debug_printf(chip8, "FX15 F%X15 Set delay timer to VX", X);
            break;

        // FX18 Set sound timer
        case 0x18:
            chip8->sound_timer = chip8->V[X];

            chip8_debug_printf(chip8, "FX18 F%X18 Set sound timmer to VX", X);
            break;

        // FX1E Add to index
        case 0x1E:
            chip8->I += chip8->V[X];

            chip8_debug_printf(chip8, "FX1E F%X1E Set I += VX", X);
            break;

        // FX0A Get key
        case 0x0A:
            // Assume no key is pressed
            // since we always do +2 after fetch, we need -2 to "block" execution of next instruction
            chip8->pc -= 2;

            // Check if a key is pressed
            // Just take the first occurence of a pressed key (since multiple can be set to 1)
            for (uint8_t i = 0; i < CHIP8_NUM_KEYS; i++)
            {
                if (chip8->keypad[i] == 1)
                {
                    chip8->V[X] = i;

                    // If a key was pressed, set it back to the next instruction
                    chip8->pc += 2;
                    break;
                }
            }

            chip8_debug_printf(chip8, "FX0A F%X0A Set VX to key pressed (blocking)", X);
            break;

        // FX29 Font character
        case 0x29:
            // Font character memory location is 0x50 + vx * 5 (each character is 5 bytes)
            chip8->I = 0x50 + chip8->V[X] * 5;

            chip8_debug_printf(chip8, "FX29 F%X29 Set I to font character VX", X);
            break;

        // FX33 Binary-coded decimal conversion
        case 0x33:
        {
            uint8_t operand = chip8->V[X];
            for (uint8_t i = 2; i >= 0; i--)
            {
                chip8->memory[chip8->I + i] = operand % 10;
                operand /= 10;
            }

            chip8_debug_printf(chip8, "FX33 F%X33 Binary-coded decimal conversion of VX into memory at I", X);
            break;
        }

        // FX55 Store V memory (from V regs to memory at I)
        case 0x55:
            for (uint8_t i = 0; i <= X; i++)
            {
                chip8->memory[chip8->I + i] = chip8->V[i];
            }

            chip8_debug_printf(chip8, "FX55 F%X55 Store V0 through VX to memory at I", X);
            break;

        // FX65 Load V memory (loads from memory at I to V regs)
        case 0x65:
            for (uint8_t i = 0; i <= X; i++)
            {
                chip8->V[i] = chip8->memory[chip8->I + i];
            }

            chip8_debug_printf(chip8, "FX65 F%X65 Load V0 through VX from memory at I", X);
            break;

        default:
            printf(CHIP8_UNKNOWN_OPCODE);
            break;
        }

        break;

    default:
        printf(CHIP8_UNKNOWN_OPCODE);
        break;
    }
}

void chip8_debug_printf(Chip8 *chip8, const char *format, ...)
{
#if CHIP8_DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n|PC %X|I %X", chip8->pc, chip8->I);
    for (uint8_t i = 0; i < CHIP8_NUM_VAR_REGISTERS; i++)
    {
        printf("|V%X %X", i, chip8->V[i]);
    }
    printf("|\n\n");
#endif
}

/// ********************
/// Stack functions    *
/// ********************

void stack_init(Stack *stack)
{
    memset(stack->arr, 0, CHIP8_STACK_SIZE * sizeof(int16_t));
    stack->top = -1;
}

void stack_push(Stack *stack, uint16_t value)
{
    stack->top++;

    if (stack->top == CHIP8_STACK_SIZE)
    {
        printf("Stack overflow");
        return;
    }
    assert(stack->top != CHIP8_STACK_SIZE && "Stack overflow in push()");

    stack->arr[stack->top] = value;
}

uint16_t stack_pop(Stack *stack)
{
    assert(stack->top != -1 && "Stack underflow in pop()");

    int popped = stack->arr[stack->top];
    stack->top--;

    return popped;
}