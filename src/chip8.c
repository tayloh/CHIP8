#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "chip8.h"

/// ********************
/// Chip8 functions    *
/// ********************

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
    printf("\n");
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

    uint8_t X = (opcode >> 8) & 0xF;
    uint8_t Y = (opcode >> 4) & 0xF;
    uint8_t N = opcode & 0xF;
    uint8_t NN = opcode & 0xFF;
    uint16_t NNN = opcode & 0xFFF;

    // printf("instruction: category=%X X=%X Y=%X N=%X NN=%X NNN=%X\n", instruction_category, X, Y, N, NN, NNN);

    // Execute instruction
    switch (instruction_category)
    {
    case 0x0:

        switch (opcode)
        {

        // 00E0 Clear screen
        case 0x00E0:
            printf("00E0 Clear screen");
            memset(chip8->display, 0, sizeof(chip8->display));
            break;

        // 00EE Return from subroutine
        case 0x00EE:
            printf("00EE");
            chip8->pc = stack_pop(&chip8->stack);
            break;

        default:
            printf("Unknown instruction");
            break;
        }

        break;

    // 1NNN Jump
    case 0x1:
        printf("1NNN Jump      - PC set to %X", NNN);
        chip8->pc = NNN;
        break;

    // 2NNN Jump to subroutine
    case 0x2:
        printf("2NNN");
        stack_push(&chip8->stack, chip8->pc);
        chip8->pc = NNN;
        break;

    // 3XNN Skip one instruction if VX == NN
    case 0x3:
        printf("3XNN Skip      - Skip one instruction if %X == %X", chip8->V[X], NN);
        if (chip8->V[X] == NN)
        {
            chip8->pc += 2;
        }
        break;

    // 4XNN Skip one instruction if VX != NN
    case 0x4:
        printf("4XNN");
        if (chip8->V[X] != NN)
        {
            chip8->pc += 2;
        }
        break;

    // 5XY0 Skip one instruction if VX == VY
    case 0x5:
        printf("5XY0");
        if (chip8->V[X] == chip8->V[Y])
        {
            chip8->pc += 2;
        }
        break;

    // 6XNN Set
    case 0x6:
        printf("6XNN Set       - Variable register at %X SET to %X", X, NN);
        chip8->V[X] = NN;
        break;

    // 7XNN Add
    case 0x7:
        printf("7XNN Add       - Variable register at %X ADD %X", X, NN);

        // CHIP8 expects overflow to happen, don't treat it
        chip8->V[X] += NN;
        break;

    case 0x8:
        printf("8XYN");
        switch (N)
        {
        // 8XY0 Set
        case 0x0:
            chip8->V[X] = chip8->V[Y];
            break;

        // 8XY1 OR
        case 0x1:
            chip8->V[X] |= chip8->V[Y];
            break;

        // 8XY2 AND
        case 0x2:
            chip8->V[X] &= chip8->V[Y];
            break;

        // 8XY3 XOR
        case 0x3:
            chip8->V[X] ^= chip8->V[Y];
            break;

        // 8XY5 Add with carry
        case 0x4:
        {
            uint16_t sum = chip8->V[X] + chip8->V[Y];
            chip8->V[0xF] = (sum > 0xFF) ? 1 : 0;
            chip8->V[X] = sum & 0xFF;
            break;
        }

        // 8XY5 Subtract with borrow (VX - VY)
        case 0x5:
            chip8->V[0xF] = (chip8->V[X] >= chip8->V[Y]) ? 1 : 0;
            chip8->V[X] = chip8->V[X] - chip8->V[Y];
            break;

        // 8XY6 Shift right
        case 0x6:
            chip8->V[0xF] = chip8->V[X] & 0x1;
            chip8->V[X] >>= 1;
            break;

        // 8XY7 Subtract with borrow (VY-VX)
        case 0x7:
            chip8->V[0xF] = (chip8->V[Y] >= chip8->V[X]) ? 1 : 0;
            chip8->V[X] = chip8->V[Y] - chip8->V[X];
            break;

        // 8XY6 Shift left
        case 0xE:
            chip8->V[0xF] = (chip8->V[X] >> 7) & 1;
            chip8->V[X] <<= 1;
            break;

        default:
            printf("Unknown instruction");
            break;
        }

        break;

    // 9XY0 Skip one instruction if VX != VY
    case 0x9:
        printf("9XY0");
        if (chip8->V[X] != chip8->V[Y])
        {
            chip8->pc += 2;
        }
        break;

    // ANNN Set index
    case 0xA:
        printf("ANNN Set index - Memory index set to %X", NNN);
        chip8->I = NNN;
        break;

    // BNNN Jump to NNN + V0
    case 0xB:
        printf("BNNN");
        chip8->pc = NNN + chip8->V[0];
        break;

    // CXNN Random
    case 0xC:
    {
        printf("CXNN");
        uint8_t random_number = (uint8_t)(rand() % 256);
        chip8->V[X] = random_number & NN;
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

        printf("DXYN Draw      - %X sprite rows drawn at (%X, %X) from memory location %X", N, x_coord, y_coord, chip8->I);

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
        /* code */
        printf("E Not implemented");
        break;

    case 0xF:
        /* code */
        printf("F Not implemented");
        break;

    default:
        printf("Unknown instruction");
        break;
    }
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