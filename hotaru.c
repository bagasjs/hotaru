#include "hotaru.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef BACKWARD_COMPATIBLE_OPENGL_MAJOR_VERSION
    #define BACKWARD_COMPATIBLE_OPENGL_MAJOR_VERSION 3
#endif 
#ifndef BACKWARD_COMPATIBLE_OPENGL_MINOR_VERSION
    #define BACKWARD_COMPATIBLE_OPENGL_MINOR_VERSION 3
#endif 
#ifndef FORWARD_COMPATIBLE_OPENGL_MAJOR_VERSION
    #define FORWARD_COMPATIBLE_OPENGL_MAJOR_VERSION 4
#endif 
#ifndef FORWARD_COMPATIBLE_OPENGL_MINOR_VERSION
    #define FORWARD_COMPATIBLE_OPENGL_MINOR_VERSION 5
#endif 

#ifndef MAXIMUM_KEYBOARD_KEYS
    #define MAXIMUM_KEYBOARD_KEYS 512
#endif
#ifndef MAXIMUM_MOUSE_BUTTONS
    #define MAXIMUM_MOUSE_BUTTONS 8
#endif
#ifndef MAXIMUM_KEYPRESSED_QUEUE
    #define MAXIMUM_KEYPRESSED_QUEUE 16
#endif

#ifndef MAXIMUM_SHADER_LOCS
    #define MAXIMUM_SHADER_LOCS 16
#endif

#define POSITION_SHADER_ATTRIBUTE_LOCATION 0
#define COLOR_SHADER_ATTRIBUTE_LOCATION 1
#define TEXCOORDS_SHADER_ATTRIBUTE_LOCATION 2
#define TEXTURE_INDEX_SHADER_ATTRIBUTE_LOCATION 3
#define TEXTURE_SAMPLERS_SHADER_UNIFORM_LOCATION 4
#define PROJECTION_MATRIX_SHADER_UNIFORM_LOCATION 5
#define VIEW_MATRIX_SHADER_UNIFORM_LOCATION 6
#define MODEL_MATRIX_SHADER_UNIFORM_LOCATION 7

#define POSITION_SHADER_ATTRIBUTE_NAME "a_Position"
#define COLOR_SHADER_ATTRIBUTE_NAME "a_Color"
#define TEXCOORDS_SHADER_ATTRIBUTE_NAME "a_TexCoords"
#define TEXTURE_INDEX_SHADER_ATTRIBUTE_NAME "a_TextureIndex"
#define TEXTURE_SAMPLERS_SHADER_UNIFORM_NAME "u_Textures"
#define PROJECTION_MATRIX_SHADER_UNIFORM_NAME "u_Projection"
#define VIEW_MATRIX_SHADER_UNIFORM_NAME "u_View"
#define MODEL_MATRIX_SHADER_UNIFORM_NAME "u_Model"

#ifndef MAXIMUM_BATCH_RENDERER_VERTICES
    #define MAXIMUM_BATCH_RENDERER_VERTICES (32*1024)
#endif
#ifndef MAXIMUM_BATCH_RENDERER_ELEMENTS
    #define MAXIMUM_BATCH_RENDERER_ELEMENTS (64*1024)
#endif
#ifndef MAXIMUM_BATCH_RENDERER_ACTIVE_TEXTURES
    #define MAXIMUM_BATCH_RENDERER_ACTIVE_TEXTURES 8
#endif

typedef struct Render_Vertex {
    struct { float x, y, z; } pos;
    struct { float r, g, b, a; } color;
    struct { float u, v; } texCoords;
    float textureIndex;
} Render_Vertex;

void dumpVertex(const Render_Vertex *vertex)
{
    TRACELOG(LOG_INFO, "pos(%f, %f, %f), color(%f, %f, %f, %f), texCoords(%f, %f), textureIndex(%f)",
            vertex->pos.x, vertex->pos.y, vertex->pos.z,
            vertex->color.r, vertex->color.g, vertex->color.b, vertex->color.a,
            vertex->texCoords.u, vertex->texCoords.v,
            vertex->textureIndex);
}

typedef struct Application_Config {
    struct {
        const char *title;
        uint32_t width, height;
        bool resizable, eventWaiting;
    } window;
    struct {
        bool enableBackwardCompatibleMode;
    } renderer;
} Application_Config;

typedef struct Application_State {
    bool initialized;
    void *userData;

    struct {
        struct {
            GLFWwindow *handle;
            uint32_t width;
            uint32_t height;
            const char *title;
            bool eventWaiting;

            bool shouldClose;
        } window;
    } desktop;

    struct {
        struct {
            char currentKeyState[MAXIMUM_KEYBOARD_KEYS];
            char previousKeyState[MAXIMUM_KEYBOARD_KEYS];

            char keyRepeatInFrame[MAXIMUM_KEYBOARD_KEYS];

            int keyPressedQueue[MAXIMUM_KEYPRESSED_QUEUE];
            int keyPressedQueueCount;
        } keyboard;
        struct {
            Vector2 offset;
            Vector2 scale;

            Vector2 currentPosition;
            Vector2 previousPosition;

            Vector2 currentWheelMove;
            Vector2 previousWheelMove;

            char currentButtonState[MAXIMUM_MOUSE_BUTTONS];
            char previousButtonState[MAXIMUM_MOUSE_BUTTONS];
        } mouse;
    } inputs;

    struct {
        struct {
            bool supportVAO;
        } config;

        uint32_t vaoID, vboID, eboID;
        Shader defaultShader;
        struct {
            Render_Vertex data[MAXIMUM_BATCH_RENDERER_VERTICES];
            uint32_t count;
        } vertices;
        struct {
            uint32_t data[MAXIMUM_BATCH_RENDERER_ELEMENTS];
            uint32_t count;
        } elements;
        struct {
            uint32_t data[MAXIMUM_BATCH_RENDERER_ACTIVE_TEXTURES];
            uint32_t count;
        } activeTextureIDs;
    } renderer;
} Application_State;

static Application_State APP = {0};
static Application_Config appConfig= {
    .window.title = "Hotaru Window",
    .window.width = 800,
    .window.height = 600,
    .window.resizable = false,
    .window.eventWaiting = false,
    .renderer.enableBackwardCompatibleMode = false,
};

void glCheckLastError(int exitOnError) 
{
    GLenum err = GL_NO_ERROR;
    printf("CHECKING OPENGL ERROR\n");
    while((err = glGetError()) != GL_NO_ERROR) {
        switch(err) {
            case GL_INVALID_ENUM: 
                fprintf(stderr, "OPENGL ERROR: GL_INVALID_ENUM\n"); 
                if(!exitOnError) { return; } else { break; }
            case GL_INVALID_VALUE: 
                fprintf(stderr, "OPENGL ERROR: GL_INVALID_VALUE\n"); 
                if(!exitOnError) { return; } else { break; }
            case GL_INVALID_OPERATION: 
                fprintf(stderr, "OPENGL ERROR: GL_INVALID_OPERATION\n"); 
                if(!exitOnError) { return; } else { break; }
            case GL_OUT_OF_MEMORY: 
                fprintf(stderr, "OPENGL ERROR: GL_OUT_OF_MEMORY\n"); 
                if(!exitOnError) { return; } else { break; }
            case GL_INVALID_FRAMEBUFFER_OPERATION: 
                fprintf(stderr, "OPENGL ERROR: GL_INVALID_FRAMEBUFFER_OPERATION\n"); 
                if(!exitOnError) { return; } else { break; }
            default: 
                fprintf(stderr, "OPENGL ERROR: UNKNOWN\n"); 
                if(!exitOnError) { return; } else { break; } 
        }
        if(exitOnError) exit(EXIT_FAILURE);
    }
    fprintf(stdout, "THERE'S NO ERROR\n");
}

static void GlfwWindowSizeCallback(GLFWwindow *window, int width, int height);
static void GlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void GlfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
static void GlfwMouseCursorPosCallback(GLFWwindow *window, double x, double y);
static void GlfwMouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

bool InitApplication(void)
{
    if(APP.initialized) return false;
    TRACELOG(LOG_INFO, "Initializing application");

    TRACELOG(LOG_INFO, "Initializing desktop platform");
    if(!glfwInit()) {
        TRACELOG(LOG_ERROR, "Failed to initialize desktop platform");
        return false;
    }

    glfwWindowHint(GLFW_RESIZABLE, appConfig.window.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, appConfig.renderer.enableBackwardCompatibleMode ? 
            GLFW_OPENGL_COMPAT_PROFILE : GLFW_OPENGL_CORE_PROFILE);
    int versionMajor = appConfig.renderer.enableBackwardCompatibleMode ? 
            BACKWARD_COMPATIBLE_OPENGL_MAJOR_VERSION : FORWARD_COMPATIBLE_OPENGL_MAJOR_VERSION;
    int versionMinor = appConfig.renderer.enableBackwardCompatibleMode ? 
            BACKWARD_COMPATIBLE_OPENGL_MINOR_VERSION : FORWARD_COMPATIBLE_OPENGL_MINOR_VERSION;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versionMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versionMinor);
    GLFWwindow *window = glfwCreateWindow(appConfig.window.width, appConfig.window.height, 
            appConfig.window.title, NULL, NULL);

    if(!window) {
        TRACELOG(LOG_ERROR, "Failed to create window");
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, GlfwWindowSizeCallback);
    glfwSetKeyCallback(window, GlfwKeyCallback);
    glfwSetMouseButtonCallback(window, GlfwMouseButtonCallback);
    glfwSetCursorPosCallback(window, GlfwMouseCursorPosCallback);
    glfwSetScrollCallback(window, GlfwMouseScrollCallback);

    APP.desktop.window.title = appConfig.window.title;
    APP.desktop.window.width = appConfig.window.width;
    APP.desktop.window.height = appConfig.window.height;
    APP.desktop.window.eventWaiting = appConfig.window.eventWaiting;
    APP.desktop.window.shouldClose = false;
    APP.desktop.window.handle = window;

    TRACELOG(LOG_ERROR, "Desktop platform initialization success");

    TRACELOG(LOG_ERROR, "Renderer initialization (OpenGL %d.%d)", versionMajor, versionMinor);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    APP.renderer.config.supportVAO = !appConfig.renderer.enableBackwardCompatibleMode;
    APP.renderer.vertices.count = 0;
    APP.renderer.elements.count = 0;
    APP.renderer.activeTextureIDs.count = 0;

    if(appConfig.renderer.enableBackwardCompatibleMode) {
        TRACELOG(LOG_INFO, "VAO is not supported");
        APP.renderer.config.supportVAO = false;
    } else {
        TRACELOG(LOG_INFO, "VAO is supported");
        APP.renderer.config.supportVAO = true;
        glGenVertexArrays(1, &APP.renderer.vaoID);
    }

    glGenBuffers(1, &APP.renderer.vboID);
    glBindBuffer(GL_ARRAY_BUFFER, APP.renderer.vboID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(APP.renderer.vertices.data), NULL, GL_DYNAMIC_DRAW);

    if(APP.renderer.config.supportVAO) {
        glBindVertexArray(APP.renderer.vaoID);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, texCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, textureIndex));
    }

    glGenBuffers(1, &APP.renderer.eboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, APP.renderer.eboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(APP.renderer.elements.data), NULL, GL_DYNAMIC_DRAW);

    TRACELOG(LOG_ERROR, "Renderer initialization success");

    APP.userData = NULL;
    APP.initialized = true;
    TRACELOG(LOG_INFO, "Application initialization success");
    return true;
}

void DeinitApplication(void)
{
    if(APP.initialized) {
        glfwDestroyWindow(APP.desktop.window.handle);
        glfwTerminate();

        APP.initialized = false;
        APP.userData = NULL;
    }
}

bool WindowShouldClose(void)
{
    return APP.desktop.window.shouldClose;
}

void SetApplicationUserData(void *ptr)
{
    APP.userData = ptr;
}

void *GetApplicationUserData(void)
{
    return APP.userData;
}

void SetWindowConfig(const char *title, uint32_t width, uint32_t height)
{
    appConfig.window.title = title;
    appConfig.window.width = width;
    appConfig.window.height = height;
}

void PollInputEvents(void)
{
    if(!APP.initialized) return;
    APP.inputs.keyboard.keyPressedQueueCount = 0;

    for (int i = 0; i < MAXIMUM_KEYBOARD_KEYS; i++) {
        APP.inputs.keyboard.previousKeyState[i] = APP.inputs.keyboard.currentKeyState[i];
        APP.inputs.keyboard.keyRepeatInFrame[i] = 0;
    }

    for (int i = 0; i < MAXIMUM_MOUSE_BUTTONS; i++) 
        APP.inputs.mouse.previousButtonState[i] = APP.inputs.mouse.currentButtonState[i];

    APP.inputs.mouse.previousPosition = APP.inputs.mouse.currentPosition;
    APP.inputs.mouse.currentPosition = (Vector2){ 0.0f, 0.0f };

    APP.inputs.mouse.previousWheelMove = APP.inputs.mouse.currentWheelMove;
    APP.inputs.mouse.currentWheelMove = (Vector2){ 0.0f, 0.0f };

    if(APP.desktop.window.eventWaiting) glfwWaitEvents();
    else glfwPollEvents();

    APP.desktop.window.shouldClose = glfwWindowShouldClose(APP.desktop.window.handle);
    glfwSetWindowShouldClose(APP.desktop.window.handle, GLFW_FALSE);
}

bool IsKeyPressed(int key)
{
    bool pressed = false;
    if((key > 0) && (key < MAXIMUM_KEYBOARD_KEYS)) {
        if ((APP.inputs.keyboard.previousKeyState[key] == 0) && (APP.inputs.keyboard.currentKeyState[key] == 1)) 
            pressed = true;
    }
    return pressed;
}

bool IsKeyDown(int key)
{
    bool down = false;
    if ((key > 0) && (key < MAXIMUM_KEYBOARD_KEYS)) {
        if (APP.inputs.keyboard.currentKeyState[key] == 1) down = true;
    }
    return down;
}

bool IsKeyReleased(int key)
{
    bool released = false;
    if ((key > 0) && (key < MAXIMUM_KEYBOARD_KEYS)) {
        if ((APP.inputs.keyboard.previousKeyState[key] == 1) && (APP.inputs.keyboard.currentKeyState[key] == 0)) 
            released = true;
    }
    return released;
}

bool IsKeyUp(int key)
{
    bool up = false;
    if ((key > 0) && (key < MAXIMUM_KEYBOARD_KEYS)) {
        if (APP.inputs.keyboard.currentKeyState[key] == 0) up = true;
    }
    return up;
}

bool IsMouseButtonPressed(int button);
bool IsMouseButtonReleased(int button);
bool IsMouseButtonDown(int button);
bool IsMouseButtonUp(int button);
int GetCursorX(void);
int GetCursorY(void);

void *HeapAlloc(size_t nBytes)
{
    void *result = malloc(nBytes);
    if(!result) return NULL;
    return result;
}

void HeapFree(void *ptr)
{
    if(ptr) free(ptr);
}

char *LoadFileText(const char *filePath)
{
    FILE *f = fopen(filePath, "r");
    if(!f) return NULL;

    fseek(f, 0L, SEEK_END);
    size_t filesz = ftell(f);
    fseek(f, 0L, SEEK_SET);
    char *result = HeapAlloc(sizeof(char) * (filesz + 1));
    if(!result) {
        fclose(f);
        return NULL;
    }

    size_t read_length = fread(result, sizeof(char), filesz, f);
    result[read_length] = '\0';
    fclose(f);
    return result;
}

void UnloadFileText(char *returnValueOfLoadFileText)
{
    HeapFree(returnValueOfLoadFileText);
}


#define LOG_MESSAGE_MAXIMUM_LENGTH (32*1024)
static const char *log_levels_as_text[] = {
    "[INFO] ",
    "[WARNING] ",
    "[FATAL] ",
};
void TraceLog(int logLevel, const char *fmt, ...)
{
    char log_message[LOG_MESSAGE_MAXIMUM_LENGTH] = {0};
    const char *log_level_text = log_levels_as_text[logLevel];
    size_t log_level_text_length = strlen(log_level_text);
    strncpy(log_message, log_level_text, log_level_text_length);
    char *result = &log_message[log_level_text_length];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(result, LOG_MESSAGE_MAXIMUM_LENGTH - log_level_text_length, fmt, ap);
    va_end(ap);
    if(logLevel == LOG_ERROR) fprintf(stderr, "%s\n", log_message);
    else printf("%s\n", log_message);
}

static void GlfwWindowSizeCallback(GLFWwindow *window, int width, int height)
{
    (void)window;
    (void)width;
    (void)height;
}

static void GlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    (void)window;
    if(key < 0) return;
    if (action == GLFW_RELEASE) APP.inputs.keyboard.currentKeyState[key] = 0;
    else if(action == GLFW_PRESS) APP.inputs.keyboard.currentKeyState[key] = 1;
    else if(action == GLFW_REPEAT) APP.inputs.keyboard.keyRepeatInFrame[key] = 1;

    // Check if there is space available in the key queue
    if ((APP.inputs.keyboard.keyPressedQueueCount < MAXIMUM_KEYPRESSED_QUEUE) && (action == GLFW_PRESS)) {
        APP.inputs.keyboard.keyPressedQueue[APP.inputs.keyboard.keyPressedQueueCount] = key;
        APP.inputs.keyboard.keyPressedQueueCount++;
    }
}

static void GlfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    (void)window;
    (void)button;
    (void)action;
    (void)mods;
}

static void GlfwMouseCursorPosCallback(GLFWwindow *window, double x, double y)
{
    (void)window;
    (void)x;
    (void)y;
}

static void GlfwMouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    APP.inputs.mouse.currentWheelMove = (Vector2){ (float)xoffset, (float)yoffset };
}

void DrawTriangle(RGBA32 color, int x1, int y1, int x2, int y2, int x3, int y3)
{
    RenderPutElement(RenderPutVertex(x1, y1, 0.0f, RGBA32_UNPACK(color), 0.0f, 0.0f, -1.0f));
    RenderPutElement(RenderPutVertex(x2, y2, 0.0f, RGBA32_UNPACK(color), 0.0f, 0.0f, -1.0f));
    RenderPutElement(RenderPutVertex(x3, y3, 0.0f, RGBA32_UNPACK(color), 0.0f, 0.0f, -1.0f));
}

void DrawRectangle(RGBA32 color, int x, int y, uint32_t w, uint32_t h)
{
    int v0 = RenderPutVertex(x, y, 0.0f, RGBA32_UNPACK(color), 0.0f, 0.0f, -1.0f);
    int v1 = RenderPutVertex(x + w, y, 0.0f, RGBA32_UNPACK(color), 0.0f, 0.0f, -1.0f);
    int v2 = RenderPutVertex(x + w, y + h, 0.0f, RGBA32_UNPACK(color), 0.0f, 0.0f, -1.0f);
    int v3 = RenderPutVertex(x, y + h, 0.0f, RGBA32_UNPACK(color), 0.0f, 0.0f, -1.0f);
    RenderPutElement(v0);
    RenderPutElement(v1);
    RenderPutElement(v2);
    RenderPutElement(v2);
    RenderPutElement(v3);
    RenderPutElement(v0);
}

void DrawTexture(Texture texture, int x, int y, uint32_t w, uint32_t h)
{
    int textureIndex = RenderEnableTexture(texture);
    int tl = RenderPutVertex(x, y, 0.0f,  
            0.0f, 0.0f, 0.0f, 0.0f, 
            0.0f, 0.0f, textureIndex);
    int tr = RenderPutVertex(x + w, y, 0.0f,  
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, textureIndex);
    int br = RenderPutVertex(x + w, y + h,  
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, textureIndex);
    int bl = RenderPutVertex(x, y + h, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, textureIndex);
    RenderPutElement(tl);
    RenderPutElement(tr);
    RenderPutElement(br);
    RenderPutElement(br);
    RenderPutElement(bl);
    RenderPutElement(tl);
}

void RenderClear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderPresent(void)
{
    glfwSwapBuffers(APP.desktop.window.handle);
}

void RenderFlush(Shader shader)
{
    if(APP.renderer.config.supportVAO) glBindVertexArray(APP.renderer.vaoID);
    if(APP.renderer.vertices.count > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, APP.renderer.vboID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(APP.renderer.vertices.data[0])*APP.renderer.vertices.count, 
                (void *)APP.renderer.vertices.data);
    }
    if(APP.renderer.elements.count > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, APP.renderer.eboID);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(APP.renderer.elements.data[0])*APP.renderer.elements.count, 
                (void *)APP.renderer.elements.data);
    }
    if(APP.renderer.config.supportVAO) glBindVertexArray(0);

    glUseProgram(shader.ID);
    if(APP.renderer.config.supportVAO) glBindVertexArray(APP.renderer.vaoID);
    else {
        glBindBuffer(GL_ARRAY_BUFFER, APP.renderer.vboID); 
        glEnableVertexAttribArray(shader.locs[POSITION_SHADER_ATTRIBUTE_LOCATION]);
        glVertexAttribPointer(shader.locs[POSITION_SHADER_ATTRIBUTE_LOCATION], 
                3, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, pos));
        glEnableVertexAttribArray(shader.locs[COLOR_SHADER_ATTRIBUTE_LOCATION]);
        glVertexAttribPointer(shader.locs[COLOR_SHADER_ATTRIBUTE_LOCATION], 
                4, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, color));
        glEnableVertexAttribArray(shader.locs[TEXCOORDS_SHADER_ATTRIBUTE_LOCATION]);
        glVertexAttribPointer(shader.locs[TEXCOORDS_SHADER_ATTRIBUTE_LOCATION], 
                2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, texCoords));
        glEnableVertexAttribArray(shader.locs[TEXTURE_INDEX_SHADER_ATTRIBUTE_LOCATION]);
        glVertexAttribPointer(shader.locs[TEXTURE_INDEX_SHADER_ATTRIBUTE_LOCATION], 
                1, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void*)offsetof(Render_Vertex, textureIndex));

        if(APP.renderer.elements.count > 0) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, APP.renderer.eboID);
    }

    int textureUnits[MAXIMUM_BATCH_RENDERER_ACTIVE_TEXTURES] = {0};
    for(int i = 0; i < MAXIMUM_BATCH_RENDERER_ACTIVE_TEXTURES; ++i) {
        textureUnits[i] = i;
    }

    for(int i = 0; i < (int)APP.renderer.activeTextureIDs.count; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, APP.renderer.activeTextureIDs.data[i]);
    }

    glUniform1iv(shader.locs[TEXTURE_SAMPLERS_SHADER_UNIFORM_LOCATION],
            MAXIMUM_BATCH_RENDERER_ACTIVE_TEXTURES, textureUnits);

    if(APP.renderer.elements.count > 0) {
        glDrawElements(GL_TRIANGLES, APP.renderer.elements.count, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, APP.renderer.vertices.count);
    }

    if(APP.renderer.config.supportVAO) glBindVertexArray(0);
    else {
        if(APP.renderer.elements.count > 0) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    glUseProgram(0);

    APP.renderer.vertices.count = 0;
    APP.renderer.elements.count = 0;
    APP.renderer.activeTextureIDs.count = 0;
}

void RenderViewport(uint32_t width, uint32_t height)
{
    glViewport(0, 0, width, height);
}

int RenderEnableTexture(Texture texture)
{
    int index = APP.renderer.activeTextureIDs.count;
    APP.renderer.activeTextureIDs.data[index] = texture.ID;
    APP.renderer.activeTextureIDs.count += 1;
    return index;
}

int RenderPutVertexWithColor(float x, float y, float z, float r, float g, float b, float a)
{
    return RenderPutVertex(x, y, z, r, g, b, a, 0.0f, 0.0f, -1.0f);
}

int RenderPutVertexWithTexture(float x, float y, float z, float u, float v, int textureIndex)
{
    return RenderPutVertex(x, y, z, 0.0f, 0.0f, 0.0f, 0.0f, u, v, textureIndex);
}

int RenderPutVertex(float x, float y, float z, float r, float g, float b, float a, float u, float v, int textureIndex)
{
    int index = APP.renderer.vertices.count;
    Render_Vertex *vertex = &APP.renderer.vertices.data[index];
    vertex->pos.x = x;
    vertex->pos.y = y;
    vertex->pos.z = z;
    vertex->color.r = r;
    vertex->color.g = g;
    vertex->color.b = b;
    vertex->color.a = a;
    vertex->texCoords.u = u;
    vertex->texCoords.v = v;
    vertex->textureIndex= (float)textureIndex;

    APP.renderer.vertices.count += 1;
    return index;
}

void RenderPutElement(int vertexIndex)
{
    APP.renderer.elements.data[APP.renderer.elements.count] = vertexIndex;
    APP.renderer.elements.count += 1;
}

bool LoadTextureFromFile(Texture *texture, const char *filePath, bool flipVerticallyOnLoad)
{
    if(!texture) return false;
    if(!filePath) return false;

    int width, height, compAmount;
    stbi_set_flip_vertically_on_load(flipVerticallyOnLoad);
    stbi_uc *data = stbi_load(filePath, &width, &height, &compAmount, 0);
    if(!data) return false;
    bool result = LoadTexture(texture, data, width, height, compAmount);
    stbi_image_free(data);
    stbi_set_flip_vertically_on_load(false);

    return result;
}

bool LoadTexture(Texture *texture, const uint8_t *data, uint32_t width, uint32_t height, uint32_t compAmount)
{
    if(!texture) return false;
    if(!data) return false;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texture->ID);
    glBindTexture(GL_TEXTURE_2D, texture->ID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, 
            compAmount== 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    texture->height = height;
    texture->width = width;
    texture->compAmount = compAmount;
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void UnloadTexture(Texture texture)
{
    glDeleteTextures(1, &texture.ID);
}

bool LoadShaderFromFile(Shader *shader, const char *vertSourceFilePath, const char *fragSourceFilePath)
{
    char *vertSource = LoadFileText(vertSourceFilePath);
    char *fragSource = LoadFileText(fragSourceFilePath);
    bool result = LoadShader(shader, vertSource, fragSource);
    UnloadFileText(vertSource);
    UnloadFileText(fragSource);
    return result;
}

bool LoadShader(Shader *shader, const char *vertSource, const char *fragSource)
{
    if(!shader) return false;
    if(!vertSource) return false;
    if(!fragSource) return false;
    uint32_t vertModule, fragModule;
    int success;

    vertModule = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertModule, 1, (const char **)&vertSource, NULL);
    glCompileShader(vertModule);
    glGetShaderiv(vertModule, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info_log[512];
        glGetShaderInfoLog(vertModule, sizeof(info_log), NULL, info_log);
        TRACELOG(LOG_ERROR, "Vertex shader compilation error \"%s\"", info_log);
        return false;
    }

    fragModule = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragModule, 1, (const char **)&fragSource, NULL);
    glCompileShader(fragModule);
    glGetShaderiv(fragModule, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info_log[512];
        glGetShaderInfoLog(fragModule, sizeof(info_log), NULL, info_log);
        glDeleteShader(vertModule);
        TRACELOG(LOG_ERROR, "Fragment shader compilation error \"%s\"", info_log);
        return false;
    }

    shader->ID = glCreateProgram();
    glAttachShader(shader->ID, vertModule);
    glAttachShader(shader->ID, fragModule);
    glLinkProgram(shader->ID);
    glGetProgramiv(shader->ID, GL_LINK_STATUS, &success);
    if(!success) {
        char info_log[512];
        glGetProgramInfoLog(shader->ID,  sizeof(info_log), NULL, info_log);
        glDeleteShader(vertModule);
        glDeleteShader(fragModule);
        TRACELOG(LOG_ERROR, "Shader Linking Error: %s\n", info_log);
        return false;
    }
    glDeleteShader(vertModule);
    glDeleteShader(fragModule);

    glUseProgram(shader->ID);
    int loc = -1;
    shader->locs = HeapAlloc(sizeof(int) * MAXIMUM_SHADER_LOCS);

#define GET_LOCATION_OF(func, name, mandatory) \
    do { \
        loc = func(*shader, (name)); \
        if(loc < 0) { \
            TRACELOG((mandatory) ? LOG_ERROR : LOG_WARNING, "Failed to find location of %s", name); \
            if(mandatory) { \
                HeapFree(shader->locs); \
                return false; \
            } \
        } \
    } while(0);

    GET_LOCATION_OF(GetShaderAttributeLocation, POSITION_SHADER_ATTRIBUTE_NAME, true);
    shader->locs[POSITION_SHADER_ATTRIBUTE_LOCATION] = loc;

    GET_LOCATION_OF(GetShaderAttributeLocation, COLOR_SHADER_ATTRIBUTE_NAME, true);
    shader->locs[COLOR_SHADER_ATTRIBUTE_LOCATION] = loc;

    GET_LOCATION_OF(GetShaderAttributeLocation, TEXCOORDS_SHADER_ATTRIBUTE_NAME, true);
    shader->locs[TEXCOORDS_SHADER_ATTRIBUTE_LOCATION] = loc;

    GET_LOCATION_OF(GetShaderAttributeLocation, TEXTURE_INDEX_SHADER_ATTRIBUTE_NAME, true);
    shader->locs[TEXTURE_INDEX_SHADER_ATTRIBUTE_LOCATION] = loc;

    GET_LOCATION_OF(GetShaderUniformLocation, TEXTURE_SAMPLERS_SHADER_UNIFORM_NAME, true);
    shader->locs[TEXTURE_SAMPLERS_SHADER_UNIFORM_LOCATION] = loc;

    GET_LOCATION_OF(GetShaderUniformLocation, PROJECTION_MATRIX_SHADER_UNIFORM_NAME, false);
    shader->locs[PROJECTION_MATRIX_SHADER_UNIFORM_LOCATION] = loc;

    GET_LOCATION_OF(GetShaderUniformLocation, VIEW_MATRIX_SHADER_UNIFORM_NAME, false);
    shader->locs[VIEW_MATRIX_SHADER_UNIFORM_LOCATION] = loc;

    GET_LOCATION_OF(GetShaderUniformLocation, MODEL_MATRIX_SHADER_UNIFORM_NAME, false);
    shader->locs[MODEL_MATRIX_SHADER_UNIFORM_LOCATION] = loc;
#undef GET_LOCATION_OF

    glUseProgram(0);
    return true;
}

void UnloadShader(Shader shader)
{
    HeapFree(shader.locs);
    glDeleteProgram(shader.ID);
}

int GetShaderUniformLocation(Shader shader, const char *uniformName)
{
    return glGetUniformLocation(shader.ID, uniformName);
}

int GetShaderAttributeLocation(Shader shader, const char *attributeName)
{
    return glGetAttribLocation(shader.ID, attributeName);
}

void SetProjectionMatrixUniform(Shader shader, Matrix matrix)
{
    SetShaderUniform(shader, shader.locs[PROJECTION_MATRIX_SHADER_UNIFORM_LOCATION],
            SHADER_UNIFORM_MAT4, (void *)matrix.elements, 1, false);
}

void SetViewMatrixUniform(Shader shader, Matrix matrix)
{
    SetShaderUniform(shader, shader.locs[VIEW_MATRIX_SHADER_UNIFORM_LOCATION],
            SHADER_UNIFORM_MAT4, (void *)matrix.elements, 1, false);
}

void SetModelMatrixUniform(Shader shader, Matrix matrix)
{
    SetShaderUniform(shader, shader.locs[MODEL_MATRIX_SHADER_UNIFORM_LOCATION],
            SHADER_UNIFORM_MAT4, (void *)matrix.elements, 1, false);
}

void SetShaderUniform(Shader shader, int location, int uniformType, const void *data, int count, bool transposeIfMatrix)
{
    glUseProgram(shader.ID);
    switch(uniformType) {
        case SHADER_UNIFORM_FLOAT:
            glUniform1fv(location, count, (const float *)data);
            break;
        case SHADER_UNIFORM_VEC2:
            glUniform2fv(location, count, (const float *)data);
            break;
        case SHADER_UNIFORM_VEC3:
            glUniform3fv(location, count, (const float *)data);
            break;
        case SHADER_UNIFORM_VEC4:
            glUniform4fv(location, count, (const float *)data);
            break;
        case SHADER_UNIFORM_UINT:
            glUniform1uiv(location, count, (const uint32_t *)data);
            break;
        case SHADER_UNIFORM_UVEC2:
            glUniform2uiv(location, count, (const uint32_t *)data);
            break;
        case SHADER_UNIFORM_UVEC3:
            glUniform3uiv(location, count, (const uint32_t *)data);
            break;
        case SHADER_UNIFORM_UVEC4:
            glUniform4uiv(location, count, (const uint32_t *)data);
            break;
        case SHADER_UNIFORM_INT:
            glUniform1iv(location, count, (const int *)data);
            break;
        case SHADER_UNIFORM_IVEC2:
            glUniform2iv(location, count, (const int *)data);
            break;
        case SHADER_UNIFORM_IVEC3:
            glUniform3iv(location, count, (const int *)data);
            break;
        case SHADER_UNIFORM_IVEC4:
            glUniform4iv(location, count, (const int *)data);
            break;
        case SHADER_UNIFORM_MAT3:
            glUniformMatrix3fv(location, count, transposeIfMatrix ? GL_TRUE : GL_FALSE, (const float *)data);
            break;
        case SHADER_UNIFORM_MAT4:
            glUniformMatrix4fv(location, count, transposeIfMatrix ? GL_TRUE : GL_FALSE, (const float *)data);
            break;
        case SHADER_UNIFORM_SAMPLER:
            glUniform1iv(location, count, data);
            break;
        default:
            break;
    }
    glUseProgram(0);
}

Matrix CreateMatrix(float eye)
{
    Matrix result = CLITERAL(Matrix){0};
    result.elements[0] = eye;
    result.elements[5] = eye;
    result.elements[10] = eye;
    result.elements[15] = eye;
    return result;
}

Matrix MatrixOrthographic(float left, float right, float bottom, float top, float far, float near)
{
    Matrix result = CreateMatrix(1.0f);
    float lr = 1.0f / (left - right);
    float bt = 1.0f / (bottom - top);
    float nf = 1.0f / (near - far);

    result.elements[0] = -2.0f * lr;
    result.elements[5] = -2.0f * bt;
    result.elements[10] = 2.0f * nf;
    result.elements[12] = (left + right) * lr;
    result.elements[13] = (top + bottom) * bt;
    result.elements[14] = (far + near) * nf;
    return result;
}

Matrix MatrixPerspective(float fovRadians, float aspectRatio, float near, float far)
{
    Matrix res = CreateMatrix(1.0f);
    float halfTanFOV = tan(fovRadians/2);
    res.elements[0] = 1.0f / (aspectRatio * halfTanFOV);
    res.elements[5] = 1.0f / halfTanFOV;
    res.elements[10] = -((far + near) / (far - near));
    res.elements[11] = -1.0f;
    res.elements[14] = -((2.0f * far * near) / (far - near));
    return res;
}

