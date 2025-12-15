#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "textrenderer.h"

// Basic 8x8 font bitmap (128 chars)
// 8 bytes per char, 1 bit per pixel
static const unsigned char font8x8_basic[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0000 (nul)
    // ... (We will generate a texture procedurally for simplicity or use a minimal set)
    // Actually, let's use a simple procedural generation for a "debug" font or just load a texture if we had one.
    // Since I cannot paste a full font header here easily without bloating, 
    // I will implement a very simple segment-like renderer OR a simple texture generator for 0-9 A-Z.
};

// Shader sources
static const char* textVertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
    "out vec2 TexCoords;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
    "    TexCoords = vertex.zw;\n"
    "}\n";

static const char* textFragmentShaderSource = "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D text;\n"
    "uniform vec3 textColor;\n"
    "void main() {\n"
    "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
    "    color = vec4(textColor, 1.0) * sampled;\n"
    "}\n";

static unsigned int textShader;
static unsigned int textVAO, textVBO;
static unsigned int fontTexture;
static mat4 textProjection;

// Simple 5x7 font data for ASCII 32-127 (Space to ~)
// Stored as bytes, 5 bytes per char.
static const unsigned char font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, // space
    0x00,0x00,0x5F,0x00,0x00, // !
    0x00,0x07,0x00,0x07,0x00, // "
    0x14,0x7F,0x14,0x7F,0x14, // #
    0x24,0x2A,0x7F,0x2A,0x12, // $
    0x23,0x13,0x08,0x64,0x62, // %
    0x36,0x49,0x55,0x22,0x50, // &
    0x00,0x05,0x03,0x00,0x00, // '
    0x00,0x1C,0x22,0x41,0x00, // (
    0x00,0x41,0x22,0x1C,0x00, // )
    0x14,0x08,0x3E,0x08,0x14, // *
    0x08,0x08,0x3E,0x08,0x08, // +
    0x00,0x50,0x30,0x00,0x00, // ,
    0x08,0x08,0x08,0x08,0x08, // -
    0x00,0x60,0x60,0x00,0x00, // .
    0x20,0x10,0x08,0x04,0x02, // /
    0x3E,0x51,0x49,0x45,0x3E, // 0
    0x00,0x42,0x7F,0x40,0x00, // 1
    0x42,0x61,0x51,0x49,0x46, // 2
    0x21,0x41,0x45,0x4B,0x31, // 3
    0x18,0x14,0x12,0x7F,0x10, // 4
    0x27,0x45,0x45,0x45,0x39, // 5
    0x3C,0x4A,0x49,0x49,0x30, // 6
    0x01,0x71,0x09,0x05,0x03, // 7
    0x36,0x49,0x49,0x49,0x36, // 8
    0x06,0x49,0x49,0x29,0x1E, // 9
    0x00,0x36,0x36,0x00,0x00, // :
    0x00,0x56,0x36,0x00,0x00, // ;
    0x08,0x14,0x22,0x41,0x00, // <
    0x14,0x14,0x14,0x14,0x14, // =
    0x00,0x41,0x22,0x14,0x08, // >
    0x02,0x01,0x51,0x09,0x06, // ?
    0x32,0x49,0x79,0x41,0x3E, // @
    0x7E,0x11,0x11,0x11,0x7E, // A
    0x7F,0x49,0x49,0x49,0x36, // B
    0x3E,0x41,0x41,0x41,0x22, // C
    0x7F,0x41,0x41,0x22,0x1C, // D
    0x7F,0x49,0x49,0x49,0x41, // E
    0x7F,0x09,0x09,0x09,0x01, // F
    0x3E,0x41,0x49,0x49,0x7A, // G
    0x7F,0x08,0x08,0x08,0x7F, // H
    0x00,0x41,0x7F,0x41,0x00, // I
    0x20,0x40,0x41,0x3F,0x01, // J
    0x7F,0x08,0x14,0x22,0x41, // K
    0x7F,0x40,0x40,0x40,0x40, // L
    0x7F,0x02,0x0C,0x02,0x7F, // M
    0x7F,0x04,0x08,0x10,0x7F, // N
    0x3E,0x41,0x41,0x41,0x3E, // O
    0x7F,0x09,0x09,0x09,0x06, // P
    0x3E,0x41,0x51,0x21,0x5E, // Q
    0x7F,0x09,0x19,0x29,0x46, // R
    0x46,0x49,0x49,0x49,0x31, // S
    0x01,0x01,0x7F,0x01,0x01, // T
    0x3F,0x40,0x40,0x40,0x3F, // U
    0x1F,0x20,0x40,0x20,0x1F, // V
    0x3F,0x40,0x38,0x40,0x3F, // W
    0x63,0x14,0x08,0x14,0x63, // X
    0x07,0x08,0x70,0x08,0x07, // Y
    0x61,0x51,0x49,0x45,0x43, // Z
    0x00,0x7F,0x41,0x41,0x00, // [
    0x02,0x04,0x08,0x10,0x20, // \ (backslash)
    0x00,0x41,0x41,0x7F,0x00, // ]
    0x04,0x02,0x01,0x02,0x04, // ^
    0x40,0x40,0x40,0x40,0x40, // _
    0x00,0x01,0x02,0x04,0x00, // `
    0x20,0x54,0x54,0x54,0x78, // a
    0x7F,0x48,0x44,0x44,0x38, // b
    0x38,0x44,0x44,0x44,0x20, // c
    0x38,0x44,0x44,0x48,0x7F, // d
    0x38,0x54,0x54,0x54,0x18, // e
    0x08,0x7E,0x09,0x01,0x02, // f
    0x0C,0x52,0x52,0x52,0x3E, // g
    0x7F,0x08,0x04,0x04,0x78, // h
    0x00,0x44,0x7D,0x40,0x00, // i
    0x20,0x40,0x44,0x3D,0x00, // j
    0x7F,0x10,0x28,0x44,0x00, // k
    0x00,0x41,0x7F,0x40,0x00, // l
    0x7C,0x04,0x18,0x04,0x78, // m
    0x7C,0x08,0x04,0x04,0x78, // n
    0x38,0x44,0x44,0x44,0x38, // o
    0x7C,0x14,0x14,0x14,0x08, // p
    0x08,0x14,0x14,0x18,0x7C, // q
    0x7C,0x08,0x04,0x04,0x08, // r
    0x48,0x54,0x54,0x54,0x20, // s
    0x04,0x3F,0x44,0x40,0x20, // t
    0x3C,0x40,0x40,0x20,0x7C, // u
    0x1C,0x20,0x40,0x20,0x1C, // v
    0x3C,0x40,0x30,0x40,0x3C, // w
    0x44,0x28,0x10,0x28,0x44, // x
    0x0C,0x50,0x50,0x50,0x3C, // y
    0x44,0x64,0x54,0x4C,0x44, // z
    0x00,0x08,0x36,0x41,0x00, // {
    0x00,0x00,0x7F,0x00,0x00, // |
    0x00,0x41,0x36,0x08,0x00, // }
    0x10,0x08,0x08,0x10,0x08  // ~
};

void initTextRenderer(int width, int height) {
    // 1. Create font texture from data
    // We'll create a 128x8 texture (all chars in a row) or similar
    // Actually, let's make a 512x8 texture to hold 96 chars * 5 width + spacing
    // Easier: Create a texture atlas. 
    // Let's just create a single texture with all chars.
    // 96 chars (32-127). Each is 5x7. We'll use 6x8 cells.
    // 16 chars per row -> 6 rows.
    // Texture size: 16*6 = 96 width, 6*8 = 48 height.
    
    int texW = 128;
    int texH = 64;
    unsigned char* texData = calloc(texW * texH, 1); // 1 byte per pixel (RED)
    
    for(int c = 0; c < 95; c++) {
        int charIdx = c; // Index in font5x7
        int gridX = c % 16;
        int gridY = c / 16;
        
        int startX = gridX * 8; // 8 pixels wide cell
        int startY = gridY * 8; // 8 pixels high cell
        
        for(int col = 0; col < 5; col++) {
            unsigned char colData = font5x7[charIdx * 5 + col];
            for(int row = 0; row < 7; row++) {
                if((colData >> row) & 1) {
                    // Flip Y for texture
                    texData[(startY + row) * texW + (startX + col)] = 255;
                }
            }
        }
    }
    
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texW, texH, 0, GL_RED, GL_UNSIGNED_BYTE, texData);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    free(texData);
    
    // 2. Compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];
    
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &textVertexShaderSource, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }
    
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &textFragmentShaderSource, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }
    
    textShader = glCreateProgram();
    glAttachShader(textShader, vertex);
    glAttachShader(textShader, fragment);
    glLinkProgram(textShader);
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    // 3. Configure VAO/VBO
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    updateTextRendererSize(width, height);
}

void updateTextRendererSize(int width, int height) {
    glm_ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f, textProjection);
    glUseProgram(textShader);
    glUniformMatrix4fv(glGetUniformLocation(textShader, "projection"), 1, GL_FALSE, (float*)textProjection);
}

void renderText(const char* text, float x, float y, float scale, vec3 color) {
    glUseProgram(textShader);
    glUniform3f(glGetUniformLocation(textShader, "textColor"), color[0], color[1], color[2]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    // Enable blending for text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // Text on top
    
    const char* p;
    for(p = text; *p; p++) {
        unsigned char c = *p;
        if(c < 32 || c > 127) c = 32;
        int charIdx = c - 32;
        
        int gridX = charIdx % 16;
        int gridY = charIdx / 16;
        
        float texW = 128.0f;
        float texH = 64.0f;
        
        // UV coordinates
        float u0 = (gridX * 8) / texW;
        float v0 = (gridY * 8) / texH;
        float u1 = (gridX * 8 + 5) / texW; // 5 pixels wide char
        float v1 = (gridY * 8 + 7) / texH; // 7 pixels high char
        
        float w = 5 * scale;
        float h = 7 * scale;
        
        float xpos = x;
        float ypos = y;
        
        float vertices[6][4] = {
            { xpos,     ypos + h,   u0, v0 },
            { xpos,     ypos,       u0, v1 },
            { xpos + w, ypos,       u1, v1 },
            
            { xpos,     ypos + h,   u0, v0 },
            { xpos + w, ypos,       u1, v1 },
            { xpos + w, ypos + h,   u1, v0 }
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        x += (6 * scale); // Advance cursor (5 width + 1 spacing)
    }
    
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
