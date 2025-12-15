#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <GLFW/glfw3.h>
#include "types.h"

// Initialise le thread de rendu
void initRenderThread(GLFWwindow* window);

// Arrête proprement le thread de rendu
void stopRenderThread();

// Met à jour les matrices de vue/projection depuis le thread principal
void updateRenderMatrices(float* view, float* projection, float camX, float camY, float camZ, int width, int height);

#endif
