#include <stdio.h>
#include "options.h"
#include "types.h"

void setRenderDistance(float distance) {
    if(distance < 2.0f) distance = 2.0f;      // Minimum 2 chunks
    if(distance > 32.0f) distance = 32.0f;    // Maximum 32 chunks
    
    game.options.renderDistance = distance;
    printf("[Options] Render distance: %.1f chunks (~%.0f blocks)\n", 
           distance, distance * CHUNK_SIZE_X);
}

void setFOV(float fov) {
    if(fov < 30.0f) fov = 30.0f;    // FOV minimum
    if(fov > 120.0f) fov = 120.0f;  // FOV maximum
    
    game.options.fov = fov;
    printf("[Options] FOV: %.1f degrees\n", fov);
}

void toggleVSync(int enabled) {
    game.options.vsync = enabled ? 1 : 0;
    printf("[Options] VSync: %s\n", enabled ? "ON" : "OFF");
    // Note: L'activation réelle de VSync nécessite glfwSwapInterval()
    // qui doit être appelé depuis le thread de rendu
}

void toggleFpsDisplay(int show) {
    game.options.showFps = show ? 1 : 0;
    printf("[Options] FPS Display: %s\n", show ? "ON" : "OFF");
}

void printGameOptions() {
    printf("\n=== Game Options ===\n");
    printf("Render Distance: %.1f chunks (~%.0f blocks)\n", 
           game.options.renderDistance, 
           game.options.renderDistance * CHUNK_SIZE_X);
    printf("FOV: %.1f degrees\n", game.options.fov);
    printf("VSync: %s\n", game.options.vsync ? "ON" : "OFF");
    printf("FPS Display: %s\n", game.options.showFps ? "ON" : "OFF");
    printf("==================\n\n");
}
