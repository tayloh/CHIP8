#include <stdio.h>
#include <string.h>

#include "chip8.h"

/// ********************
/// Chip8 functions    *
/// ********************
void chip8_init(Chip8 *chip8)
{
    memset(chip8->memory, 0, sizeof(chip8->memory));
    memset(chip8->V, 0, sizeof(chip8->V));
    memset(chip8->display, 0, sizeof(chip8->display));
    memset(chip8->keypad, 0, sizeof(chip8->keypad));

    stack_init(&chip8->stack);

    chip8->I = 0;

    // Program starts at 0x200
    chip8->pc = 0x200;

    chip8->delay_timer = 0;
    chip8->sound_timer = 0;
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

    // CHIP-8 instructions are divided into broad categories by the first “nibble”, or “half-byte”,
    // which is the first hexadecimal number.

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
            // TODO
            break;

        default:
            break;
        }

        break;

    // 1NNN
    case 0x1:
        printf("1NNN Jump      - PC set to %X", NNN);
        chip8->pc = NNN;
        break;

    case 0x2:
        /* code */
        break;

    case 0x3:
        /* code */
        break;

    case 0x4:
        /* code */
        break;

    case 0x5:
        /* code */
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
        /* code */
        break;

    case 0x9:
        /* code */
        break;

    // ANNN Set index
    case 0xA:
        printf("ANNN Set index - Memory index set to %X", NNN);
        chip8->I = NNN;
        break;

    case 0xB:
        /* code */
        break;

    case 0xC:
        /* code */
        break;

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

            // Skip empty spite rows
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
        break;

    case 0xF:
        /* code */
        break;

    default:
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

uint16_t stacK_pop(Stack *stack)
{
    assert(stack->top != -1 && "Stack underflow in pop()");

    int popped = stack->arr[stack->top];
    stack->top--;

    return popped;
}