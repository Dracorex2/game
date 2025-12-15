#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include "types.h"
#include "world.h"
#include "chunk.h"
#include "texture.h"
#include "raycast.h"
#include "renderer.h"
#include "camera.h"
#include "renderthread.h"
#include "options.h"
#include "init_blocks_entities.h"

// Instance globale du contexte de jeu
GameContext game = {0};

int main() {
    GLFWwindow* window;
    initGLFW(&window);
    
    // Initialiser les valeurs par défaut de la caméra
    game.plPos[X] = 0.0f;
    game.plPos[Y] = 20.0f;  // Spawner au-dessus du terrain
    game.plPos[Z] = 0.0f;
    game.cameraFront[X] = 0.0f;
    game.cameraFront[Y] = 0.0f;
    game.cameraFront[Z] = -1.0f;
    game.cameraUp[X] = 0.0f;
    game.cameraUp[Y] = 1.0f;
    game.cameraUp[Z] = 0.0f;
    game.yaw = -90.0f;
    game.pitch = 0.0f;
    game.velocityY = 0.0f;
    game.deltaTime = 0.0f;
    game.lastFrame = 0.0f;
    
    // Initialiser les options de jeu
    game.options.renderDistance = 12.0f;  // 12 chunks de distance (~192 blocs)
    game.options.fov = 60.0f;             // 60 degrés de FOV
    game.options.vsync = 0;               // VSync désactivé par défaut
    game.options.showFps = 1;             // Afficher les FPS
    
    game.selectedBlockID = 1;             // Default block (Stone)
    
    // Initialiser le monde AVANT de créer l'atlas
    // (charge les définitions de blocs depuis blocks.block)

    initBlocksEntities();
    initWorld();
    
    // Créer l'atlas de textures sur le thread principal
    // (doit être fait avant d'initialiser le thread de rendu)
    createTextureAtlas();
    
    // Démarrer le thread de rendu
    // Le contexte OpenGL sera transféré au thread de rendu
    initRenderThread(window);
    
    // Afficher les options et les commandes
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║          Minecraft-like - Controls & Options         ║\n");
    printf("╠═══════════════════════════════════════════════════════╣\n");
    printf("║ Movement:  WASD + Mouse  |  Jump: SPACE              ║\n");
    printf("║ Place: Right Click       |  Break: Left Click        ║\n");
    printf("║───────────────────────────────────────────────────────║\n");
    printf("║ F1: Decrease Render Distance (-2 chunks)             ║\n");
    printf("║ F2: Increase Render Distance (+2 chunks)             ║\n");
    printf("║ F3: Toggle FPS Display                                ║\n");
    printf("║ F4: Show Current Options                              ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printGameOptions();
    
    printf("[Main] Boucle principale démarrée (logique du jeu)\n");
    
    // Initialiser lastFrame pour éviter un deltaTime énorme à la première frame
    game.lastFrame = glfwGetTime();
    
    // Boucle principale : logique du jeu uniquement
    while(!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        game.deltaTime = currentFrame - game.lastFrame;
        game.lastFrame = currentFrame;

        // Traiter les entrées utilisateur
        processInput(window);
        
        // Calculer les matrices view/projection
        mat4 view, projection;
        vec3 cameraViewPos = {game.plPos[X], game.plPos[Y] + EYE_HEIGHT, game.plPos[Z]};
        vec3 target;
        glm_vec3_add(cameraViewPos, game.cameraFront, target);
        glm_lookat(cameraViewPos, target, game.cameraUp, view);
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm_perspective(glm_rad(game.options.fov), (float)width / (float)height, 0.1f, 100.0f, projection);
        
        // Envoyer les matrices au thread de rendu
        updateRenderMatrices((float*)view, (float*)projection, 
                           game.plPos[X], game.plPos[Y], game.plPos[Z],
                           width, height);
        
        // ICI : Plus tard tu pourras ajouter la logique du jeu
        // - Mise à jour des mobs
        // - Physique
        // - IA
        // - etc.
        
        // Traiter les événements GLFW (clavier, souris)
        glfwPollEvents();
        
        // Petit sleep pour ne pas saturer le CPU (16ms = ~60 FPS)
        struct timespec ts = {0, 16000000}; // 16 millisecondes
        nanosleep(&ts, NULL);
    }

    // Arrêter proprement le thread de rendu
    stopRenderThread();
    
    freeTextures();
    freeWorld();
    glfwTerminate();
    
    return 0;
}
