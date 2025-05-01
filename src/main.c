#include <stdio.h>
#include <stdint.h>

#include "chip8.h"
#include "raylib.h"

#define CYCLES_PER_SECOND 700
#define REFRESH_RATE 60
#define CHIP8_DISPLAY_SCALE 10

int main(int argc, char *argv[])
{

    // Init chip8
    if (argc < 2)
    {
        printf("Usage: chip8 <rom_file>\n");
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

    // --

    // Init raylib
    InitWindow(CHIP8_SCREEN_WIDTH * CHIP8_DISPLAY_SCALE, CHIP8_SCREEN_HEIGHT * CHIP8_DISPLAY_SCALE, "taylohs CHIP8");

    // Create a blank texture
    Image image = GenImageColor(CHIP8_SCREEN_WIDTH, CHIP8_SCREEN_HEIGHT, BLACK);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image); // Don't need image after loading texture

    SetTargetFPS(1);

    while (!WindowShouldClose())
    {
        // for (int i = 0; i < CYCLES_PER_SECOND / REFRESH_RATE; i++)
        // {
        //     chip8_cycle(&chip8);
        // }
        chip8_cycle(&chip8);
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
    }

    UnloadTexture(texture);
    CloseWindow();

    return 0;
}