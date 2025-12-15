#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>
#include "types.h"

// GLFW initialization
void initGLFW(GLFWwindow **window);

// Shader and rendering functions
unsigned int createShaderProgram();
unsigned int createCrosshairShader();
unsigned int createCrosshairVAO();
void drawWorld(unsigned int shader);
void drawTileEntities(unsigned int shader);
void drawCrosshair(unsigned int shader, unsigned int VAO);

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

#endif
