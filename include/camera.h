#ifndef CAMERA_H
#define CAMERA_H

#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include "types.h"

// Camera functions
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

#endif
