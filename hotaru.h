#ifndef HOTARU_H_
#define HOTARU_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HT_PLATFORM_DESKTOP

#define TRACELOG(level, ...) TraceLog((level), __VA_ARGS__)
#ifdef __cplusplus
    #define CLITERAL(T) T
#else
    #define CLITERAL(T) (T)
#endif

#define RGBA32_UNPACK(c) ((float)(c).r/255.0f),((float)(c).g/255.0f),((float)(c).b/255.0f),((float)(c).a/255.0f)
#define BLACK CLITERAL(RGBA32){ .r=0x00, .g=0x00, .b=0x00, .a=0xFF }
#define WHITE CLITERAL(RGBA32){ .r=0xFF, .g=0xFF, .b=0xFF, .a=0xFF }
#define RED   CLITERAL(RGBA32){ .r=0xFF, .g=0x00, .b=0x00, .a=0xFF }
#define GREEN CLITERAL(RGBA32){ .r=0x00, .g=0xFF, .b=0x00, .a=0xFF }
#define BLUE  CLITERAL(RGBA32){ .r=0x00, .g=0x00, .b=0xFF, .a=0xFF }

typedef struct Vector2 {
    float x, y;
} Vector2;

typedef struct Vector3 {
    float x, y, z;
} Vector3;

typedef struct Vector4 {
    float x, y, z, w;
} Vector4;

typedef struct Matrix {
    float elements[4*4];
} Matrix;

typedef struct Shader {
    uint32_t ID;
    int *locs;
} Shader;

typedef struct Texture {
    uint32_t ID;
    uint32_t width, height;
    uint32_t compAmount; // RGBA = 4, RGB = 3
} Texture;

typedef struct RGBA32 {
    uint8_t r, g, b, a;
} RGBA32;

/*******************************
 * Application related functions
 *******************************/
void SetWindowConfig(const char *title, uint32_t width, uint32_t height);
void SetApplicationConfig(int configKey, int configValue);
bool InitApplication(void);
void DeinitApplication(void);
void SetApplicationUserData(void *ptr);
void *GetApplicationUserData(void);

/*******************************
 * Event handling
 *******************************/
void PollInputEvents(void);
bool WindowShouldClose(void);
bool IsKeyPressed(int key);
bool IsKeyReleased(int key);
bool IsKeyDown(int key);
bool IsKeyUp(int key);
bool IsMouseButtonPressed(int button);
bool IsMouseButtonReleased(int button);
bool IsMouseButtonDown(int button);
bool IsMouseButtonUp(int button);
int GetCursorX(void);
int GetCursorY(void);

/*******************************
 * Low level graphics functions
 *******************************/
void RenderClear(float r, float g, float b, float a);
void RenderPresent(void);
void RenderFlush(Shader shader);
void RenderViewport(uint32_t width, uint32_t height);
int RenderEnableTexture(Texture texture);
int RenderPutVertex(float x, float y, float z, float r, float g, float b, float a, float u, float v, int textureIndex);
int RenderPutVertexWithColor(float x, float y, float z, float r, float g, float b, float a);
int RenderPutVertexWithTexture(float x, float y, float z, float u, float v, int textureIndex);
void RenderPutElement(int vertexIndex);

/*******************************
 * Graphics related functions
 *******************************/
bool LoadTexture(Texture *result, const uint8_t *data, uint32_t width, uint32_t height, uint32_t compAmount);
bool LoadTextureFromFile(Texture *texture, const char *filePath, bool flipVerticallyOnLoad);
void UnloadTexture(Texture texture);

bool LoadShader(Shader *result, const char *vertSource, const char *fragSource);
bool LoadShaderFromFile(Shader *result, const char *vertSourceFilePath, const char *fragSourceFilePath);
void UnloadShader(Shader shader);
int GetShaderUniformLocation(Shader shader, const char *uniformName);
int GetShaderAttributeLocation(Shader shader, const char *attributeName);
void SetProjectionMatrixUniform(Shader shader, Matrix matrix);
void SetViewMatrixUniform(Shader shader, Matrix matrix);
void SetModelMatrixUniform(Shader shader, Matrix matrix);
void SetShaderUniform(Shader shader, int location, int uniformType, const void *data, int count, bool transposeIfMatrix);

void DrawTriangle(RGBA32 color, int x1, int y1, int x2, int y2, int x3, int y3);
void DrawRectangle(RGBA32 color, int x, int y, uint32_t width, uint32_t height);
void DrawTexture(Texture texture, int x, int y, uint32_t width, uint32_t height);

/*******************************
 * Non-application dependent functions
 *******************************/
void *HeapAlloc(size_t nBytes);
void HeapFree(void *ptr);
void TraceLog(int logLevel, const char *fmt, ...);
void glCheckLastError(int exitOnError);
char *LoadFileText(const char *filePath);
void UnloadFileText(char *returnValueOfLoadFileText);

/**************************************
 * Simple vector and matrix operations 
 **************************************/
Matrix CreateMatrix(float eye);
Matrix MatrixOrthographic(float left, float right, float bottom, float top, float far, float near);
Matrix MatrixPerspective(float fovRadians, float aspectRatio, float near, float far);

/*******************************
 * Enumerations
 *******************************/
enum Shader_Uniform_Type {
    INVALID_SHADER_UNIFORM = 0,
    SHADER_UNIFORM_FLOAT, SHADER_UNIFORM_VEC2, SHADER_UNIFORM_VEC3, SHADER_UNIFORM_VEC4,
    SHADER_UNIFORM_UINT, SHADER_UNIFORM_UVEC2, SHADER_UNIFORM_UVEC3, SHADER_UNIFORM_UVEC4,
    SHADER_UNIFORM_INT, SHADER_UNIFORM_IVEC2, SHADER_UNIFORM_IVEC3, SHADER_UNIFORM_IVEC4,
    SHADER_UNIFORM_MAT3, SHADER_UNIFORM_MAT4, SHADER_UNIFORM_SAMPLER,
};

enum Log_Level {
    LOG_INFO = 0,
    LOG_WARNING,
    LOG_ERROR,
};

enum Application_Config_Key {
    INVALID_APPLICATION_CONFIG,
    APPLICATION_CONFIG_RENDERER_BACKWARD_COMPATIBLE,
};

#endif  // HOTARU_H_
