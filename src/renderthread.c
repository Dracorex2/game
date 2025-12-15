#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "renderthread.h"
#include "renderer.h"
#include "chunk.h"
#include "types.h"
#include "textrenderer.h"

// Variables locales au thread de rendu
static GLFWwindow* renderWindow = NULL;
static unsigned int shaderProgram = 0;
static unsigned int crosshairShader = 0;
static unsigned int crosshairVAO = 0;

// Fonction principale du thread de rendu
static void* renderThreadFunc(void* arg) {
    (void)arg;
    
    printf("[RenderThread] Thread de rendu démarré\n");
    
    // Le contexte OpenGL doit être activé sur ce thread
    glfwMakeContextCurrent(renderWindow);
    
    // Créer les shaders et VAOs dans le contexte du thread de rendu
    shaderProgram = createShaderProgram();
    crosshairShader = createCrosshairShader();
    
    // Créer le VAO du curseur
    float crosshairVertices[] = {
        -0.02f, 0.0f, 0.0f,
         0.02f, 0.0f, 0.0f,
         0.0f, -0.03f, 0.0f,
         0.0f,  0.03f, 0.0f
    };
    unsigned int VBO;
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    initTextRenderer(800, 600);

    printf("[RenderThread] Shaders et VAOs créés\n");
    
    // Variables pour le compteur de FPS
    int fpsCounter = 0;
    int currentFPS = 0;
    double lastFpsTime = glfwGetTime();
    
    // Boucle de rendu
    while(1) {
        pthread_mutex_lock(&game.renderThread->mutex);
        
        // Vérifier si on doit arrêter
        if(game.renderThread->shouldExit) {
            pthread_mutex_unlock(&game.renderThread->mutex);
            break;
        }
        
        // Copier les données de rendu localement
        float view[16], projection[16];
        memcpy(view, game.renderThread->viewMatrix, sizeof(view));
        memcpy(projection, game.renderThread->projectionMatrix, sizeof(projection));
        int width = game.renderThread->framebufferWidth;
        int height = game.renderThread->framebufferHeight;
        float camX = game.renderThread->cameraX;
        float camY = game.renderThread->cameraY;
        float camZ = game.renderThread->cameraZ;
        int selectedBlockID = game.renderThread->selectedBlockID;
        
        pthread_mutex_unlock(&game.renderThread->mutex);
        
        // Rendu (sans mutex pour ne pas bloquer le thread principal)
        glViewport(0, 0, width, height);
        // Couleur du ciel (Sky Blue)
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Dessiner le monde
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection);
        
        // Bind texture array et envoyer uniforms d'animation
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, game.textureAtlas);
        glUniform1i(glGetUniformLocation(shaderProgram, "blockTexture"), 0);
        glUniform1f(glGetUniformLocation(shaderProgram, "time"), (float)glfwGetTime());
        glUniform1i(glGetUniformLocation(shaderProgram, "maxFrames"), game.atlasMaxFrames);
        
        // Debug: vérifier que textureAtlas est valide
        static int frameCount = 0;
        if(frameCount % 120 == 0) {  // Tous les ~2 secondes
            GLint boundTex;
            glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &boundTex);
            // printf("[RenderThread frame %d] textureAtlas=%u, bound=%d, maxFrames=%d\n", 
            //        frameCount, game.textureAtlas, boundTex, game.atlasMaxFrames);
        }
        frameCount++;
        
        // Envoyer tableau animFrames
        static int debugOnce = 0;
        int animFramesArray[64] = {0};
        for(int i = 0; i < game.blockCount && i < 64; i++) {
            animFramesArray[i] = game.blocks[i].animFrames;
        }
        
        if(!debugOnce) {
            printf("[RenderThread] Envoi animFrames array:\n");
            for(int i = 0; i < game.blockCount; i++) {
                printf("  animFrames[%d] = %d (%s)\n", i, animFramesArray[i], game.blocks[i].name);
            }
            printf("[RenderThread] maxFrames = %d, textureAtlas = %u\n", game.atlasMaxFrames, game.textureAtlas);
            debugOnce = 1;
        }
        glUniform1iv(glGetUniformLocation(shaderProgram, "animFrames"), 64, animFramesArray);
        
        drawWorld(shaderProgram);
        
        // Dessiner le curseur
        glUseProgram(crosshairShader);
        glDisable(GL_DEPTH_TEST);
        drawCrosshair(crosshairShader, crosshairVAO);
        
        // Dessiner le HUD (Texte)
        updateTextRendererSize(width, height);
        
        // FPS
        if(game.options.showFps) {
            char fpsText[32];
            snprintf(fpsText, 32, "FPS: %d", currentFPS);
            renderText(fpsText, 10.0f, height - 20.0f, 2.0f, (vec3){1.0f, 1.0f, 0.0f});
        }
        
        // Bloc sélectionné
        if(selectedBlockID >= 0 && selectedBlockID < game.blockCount) {
            char blockText[64];
            snprintf(blockText, 64, "Block: %s", game.blocks[selectedBlockID].name);
            renderText(blockText, 10.0f, 10.0f, 2.0f, (vec3){1.0f, 1.0f, 1.0f});
            
            // Instructions
            renderText("< > to cycle", 10.0f, 30.0f, 1.5f, (vec3){0.8f, 0.8f, 0.8f});
        }
        
        glEnable(GL_DEPTH_TEST);
        
        // Swap buffers
        glfwSwapBuffers(renderWindow);
        
        // Compteur de FPS
        fpsCounter++;
        double currentTime = glfwGetTime();
        if(currentTime - lastFpsTime >= 1.0) {
            currentFPS = fpsCounter;
            if(game.options.showFps) {
                printf("FPS: %d\n", fpsCounter);
            }
            fpsCounter = 0;
            lastFpsTime = currentTime;
        }
        
        // Petit sleep pour limiter la fréquence si besoin (optionnel)
        // usleep(1000); // 1ms
    }
    
    printf("[RenderThread] Thread de rendu arrêté\n");
    return NULL;
}

void initRenderThread(GLFWwindow* window) {
    renderWindow = window;
    
    // Allouer la structure renderThread
    game.renderThread = malloc(sizeof(RenderThreadState));
    if(!game.renderThread) {
        fprintf(stderr, "Erreur: impossible d'allouer renderThread\n");
        exit(1);
    }
    
    // Initialiser le mutex
    pthread_mutex_init(&game.renderThread->mutex, NULL);
    
    // Initialiser l'état
    game.renderThread->running = 1;
    game.renderThread->shouldExit = 0;
    game.renderThread->framebufferWidth = 800;
    game.renderThread->framebufferHeight = 600;
    
    // Initialiser les matrices à l'identité
    for(int i = 0; i < 16; i++) {
        game.renderThread->viewMatrix[i] = 0.0f;
        game.renderThread->projectionMatrix[i] = 0.0f;
    }
    game.renderThread->viewMatrix[0] = game.renderThread->viewMatrix[5] = game.renderThread->viewMatrix[10] = game.renderThread->viewMatrix[15] = 1.0f;
    game.renderThread->projectionMatrix[0] = game.renderThread->projectionMatrix[5] = game.renderThread->projectionMatrix[10] = game.renderThread->projectionMatrix[15] = 1.0f;
    
    // Détacher le contexte OpenGL du thread principal
    // Le thread de rendu va le récupérer
    glfwMakeContextCurrent(NULL);
    
    // Créer le thread de rendu
    if(pthread_create(&game.renderThread->thread, NULL, renderThreadFunc, NULL) != 0) {
        fprintf(stderr, "Erreur: impossible de créer le thread de rendu\n");
        exit(1);
    }
    
    printf("[Main] Thread de rendu initialisé\n");
}

void stopRenderThread() {
    printf("[Main] Arrêt du thread de rendu...\n");
    
    // Signaler au thread qu'il doit s'arrêter
    pthread_mutex_lock(&game.renderThread->mutex);
    game.renderThread->shouldExit = 1;
    pthread_mutex_unlock(&game.renderThread->mutex);
    
    // Attendre que le thread se termine
    pthread_join(game.renderThread->thread, NULL);
    
    // Détruire le mutex
    pthread_mutex_destroy(&game.renderThread->mutex);
    
    printf("[Main] Thread de rendu arrêté\n");
}

void updateRenderMatrices(float* view, float* projection, float camX, float camY, float camZ, int width, int height) {
    pthread_mutex_lock(&game.renderThread->mutex);
    
    memcpy(game.renderThread->viewMatrix, view, 16 * sizeof(float));
    memcpy(game.renderThread->projectionMatrix, projection, 16 * sizeof(float));
    game.renderThread->cameraX = camX;
    game.renderThread->cameraY = camY;
    game.renderThread->cameraZ = camZ;
    game.renderThread->framebufferWidth = width;
    game.renderThread->framebufferHeight = height;
    game.renderThread->selectedBlockID = game.selectedBlockID;
    
    pthread_mutex_unlock(&game.renderThread->mutex);
}
