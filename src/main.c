#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "raylib.h"

#define CYCLES_PER_SECOND 700
#define REFRESH_RATE 60
#define CHIP8_DISPLAY_SCALE 20

void get_input(uint8_t input[])
{

    // Map this
    // 1 	2 	3 	4
    // Q 	W 	E 	R
    // A 	S 	D 	F
    // Z 	X 	C 	V

    // to this
    // 1 	2 	3 	C
    // 4 	5 	6 	D
    // 7 	8 	9 	E
    // A 	0 	B 	F

    memset(input, 0, sizeof(uint8_t) * CHIP8_NUM_KEYS);

    if (IsKeyDown(KEY_ONE))
        input[0x1] = 1;

    if (IsKeyDown(KEY_TWO))
        input[0x2] = 1;

    if (IsKeyDown(KEY_THREE))
        input[0x3] = 1;

    if (IsKeyDown(KEY_FOUR))
        input[0xC] = 1;

    if (IsKeyDown(KEY_Q))
        input[0x4] = 1;

    if (IsKeyDown(KEY_W))
        input[0x5] = 1;

    if (IsKeyDown(KEY_E))
        input[0x6] = 1;

    if (IsKeyDown(KEY_R))
        input[0xD] = 1;

    if (IsKeyDown(KEY_A))
        input[0x7] = 1;

    if (IsKeyDown(KEY_S))
        input[0x8] = 1;

    if (IsKeyDown(KEY_D))
        input[0x9] = 1;

    if (IsKeyDown(KEY_F))
        input[0xE] = 1;

    if (IsKeyDown(KEY_Z))
        input[0xA] = 1;

    if (IsKeyDown(KEY_X))
        input[0x0] = 1;

    if (IsKeyDown(KEY_C))
        input[0xB] = 1;

    if (IsKeyDown(KEY_V))
        input[0xF] = 1;
}

int main(int argc, char *argv[])
{

    // Init chip8
    if (argc < 3)
    {
        printf("Usage: chip8 <rom_file> <clock frequency>\n");
        return 1;
    }

    Chip8 chip8;
    chip8_init(&chip8);

    int ret = chip8_load_rom(&chip8, argv[1]);
    if (ret != 0)
    {
        printf("Error: Could not load ROM file '%s'. Exiting...\n", argv[1]);
        return 1;
    }

    const int clock_frequency = atoi(argv[2]);

    // --

    // Init raylib
    InitWindow(CHIP8_SCREEN_WIDTH * CHIP8_DISPLAY_SCALE, CHIP8_SCREEN_HEIGHT * CHIP8_DISPLAY_SCALE, "taylohs CHIP8");

    // Create a blank texture
    Image image = GenImageColor(CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image); // Don't need image after loading texture

    InitAudioDevice();
    Sound beep = LoadSound("assets/beep.wav");

    SetTargetFPS(REFRESH_RATE);

    // For timers
    float timer_accumulator = 0.0f;
    const float timer_interval = 1.0f / 60.0f;

    // For input
    uint8_t input_array[CHIP8_NUM_KEYS];

    while (!WindowShouldClose())
    {

        // Handle input
        get_input(input_array);
        chip8_pass_input(&chip8, input_array);

        // Handle cycle
        for (int i = 0; i < clock_frequency / REFRESH_RATE; i++)
        {
            chip8_cycle(&chip8);
        }

        // Handle timers
        float frame_time = GetFrameTime();
        timer_accumulator += frame_time;

        while (timer_accumulator >= timer_interval)
        {
            if (chip8.delay_timer > 0)
                chip8.delay_timer--;
            if (chip8.sound_timer > 0)
                chip8.sound_timer--;

            timer_accumulator -= timer_interval;
        }

        // Draw to window
        Color pixels[CHIP8_SCREEN_WIDTH * CHIP8_SCREEN_HEIGHT];

        for (int y = 0; y < CHIP8_SCREEN_HEIGHT; y++)
        {
            for (int x = 0; x < CHIP8_SCREEN_WIDTH; x++)
            {
                int index = y * CHIP8_SCREEN_WIDTH + x;
                pixels[index] = chip8.display[index] ? WHITE : BLACK;
            }
        }

        // Upload pixel array into the GPU texture
        UpdateTexture(texture, pixels);

        BeginDrawing();
        DrawTextureEx(texture, (Vector2){0, 0}, 0.0f, CHIP8_DISPLAY_SCALE, WHITE);
        EndDrawing();

        if (chip8.sound_timer > 0)
        {
            PlaySound(beep);
        }
    }

    UnloadTexture(texture);
    UnloadSound(beep);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}