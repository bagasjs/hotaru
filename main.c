#include "hotaru.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WORLD_SPEED 5

int main(void)
{
    SetWindowConfig("My Window", WINDOW_WIDTH, WINDOW_HEIGHT);
    InitApplication();

    Shader shader;
    if(!LoadShaderFromFile(&shader, "./res/main.vert", "./res/main.frag")) return -1;
    TraceLog(LOG_INFO, "Loaded shader ID: %u", shader.ID);

    Texture ikanTexture;
    const char *ikanFilePath = "./res/ikan.jpg";
    if(!LoadTextureFromFile(&ikanTexture, ikanFilePath, false)){
        TraceLog(LOG_ERROR, "Failed to load %s", ikanFilePath);
        return -1;
    }
    TraceLog(LOG_INFO, "Loaded texture ID: %u", ikanTexture.ID);

    Matrix projection = MatrixOrthographic(0.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);
    SetProjectionMatrixUniform(shader, projection);

    int ikanPosX = 300;
    int ikanPosY = 300;

    while(!WindowShouldClose()) {
        PollInputEvents();
        if(IsKeyDown('W')) ikanPosY -= WORLD_SPEED;
        if(IsKeyDown('A')) ikanPosX -= WORLD_SPEED;
        if(IsKeyDown('S')) ikanPosY += WORLD_SPEED;
        if(IsKeyDown('D')) ikanPosX += WORLD_SPEED;

        RenderClear(RGBA32_UNPACK(BLACK));
        DrawRectangle(RED, 0, 0, 200, 100);
        DrawTexture(ikanTexture, ikanPosX, ikanPosY, 200, 200);
        RenderFlush(shader);
        RenderPresent();
    }

    DeinitApplication();
}

