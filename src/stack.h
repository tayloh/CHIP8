#ifndef STACK_H
#define STACK_H

#include <stdint.h>
#include <assert.h>

enum
{
    CHIP8_STACK_MAX_SIZE = 256
};

typedef struct
{
    uint16_t arr[CHIP8_STACK_MAX_SIZE];
    uint8_t top;
} Stack;

void init(Stack *stack)
{
    stack->top = -1;
}

void push(Stack *stack, uint16_t value)
{
    stack->top++;

    if (stack->top == CHIP8_STACK_MAX_SIZE)
    {
        printf("Stack overflow");
        return;
    }
    assert(stack->top != CHIP8_STACK_MAX_SIZE && "Stack overflow in push()");

    stack->arr[stack->top] = value;
}

uint16_t pop(Stack *stack)
{
    assert(stack->top != -1 && "Stack underflow in pop()");

    int popped = stack->arr[stack->top];
    stack->top--;

    return popped;
}

#endif STACK_H