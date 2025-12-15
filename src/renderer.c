#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cglm/cglm.h>
#include "renderer.h"
#include "chunk.h"
#include "entities.h"

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

void initGLFW(GLFWwindow **window) {
    if (!glfwInit()) exit(-1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Minecraft-like", NULL, NULL);
    if (!*window) { glfwTerminate(); exit(-1); }

    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, framebuffer_size_callback);
    glfwSetCursorPosCallback(*window, mouse_callback);
    glfwSetMouseButtonCallback(*window, mouse_button_callback);
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Erreur : GLAD n'a pas pu charger OpenGL\n");
        exit(-1);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

unsigned int createShaderProgram() {
    const char* vertexShaderSource = "#version 330 core\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec2 aTexCoord;\n"
    "layout(location=2) in float aBlockType;\n"
    "out vec2 TexCoord;\n"
    "out float BlockType;\n"
    "uniform mat4 model, view, projection;\n"
    "void main(){ gl_Position = projection * view * model * vec4(aPos,1.0); TexCoord = aTexCoord; BlockType = aBlockType; }\n";

    const char* fragmentShaderSource = "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "in float BlockType;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2DArray blockTexture;\n"
    "uniform float time;\n"
    "uniform int animFrames[64];\n"
    "uniform int maxFrames;\n"
    "void main(){\n"
    "    int blockType = int(BlockType);\n"
    "    int frames = (blockType > 0 && blockType < 64) ? animFrames[blockType] : 1;\n"
    "    float frame = (frames > 1) ? mod(floor(time * 8.0), float(frames)) : 0.0;\n"
    "    float layer = float((blockType - 1) * maxFrames) + frame;\n"
    "    vec4 texColor = texture(blockTexture, vec3(TexCoord, layer));\n"
    "    if(texColor.a < 0.1) discard;\n"
    "    FragColor = texColor;\n"
    "}\n";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

unsigned int createCrosshairShader() {
    const char* vertexShaderSource = "#version 330 core\n"
    "layout(location=0) in vec3 aPos;\n"
    "uniform mat4 model;\n"
    "void main(){ gl_Position = vec4(aPos, 1.0); }\n";

    const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main(){ FragColor = vec4(1.0, 1.0, 1.0, 1.0); }\n";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Fonction pour vérifier si un chunk est dans le frustum et à portée
static inline int isChunkVisible(int cx, int cz, float camX, float camZ) {
    // Position du centre du chunk
    float chunkCenterX = (cx * CHUNK_SIZE_X) + (CHUNK_SIZE_X / 2.0f);
    float chunkCenterZ = (cz * CHUNK_SIZE_Z) + (CHUNK_SIZE_Z / 2.0f);
    
    // Distance au chunk
    float dx = chunkCenterX - camX;
    float dz = chunkCenterZ - camZ;
    float distSq = dx * dx + dz * dz;
    
    // Render distance (configurable via game.options)
    float maxRenderDistanceChunks = game.options.renderDistance;
    float maxDistSq = (maxRenderDistanceChunks * CHUNK_SIZE_X) * (maxRenderDistanceChunks * CHUNK_SIZE_X);
    
    // Culling par distance
    return distSq < maxDistSq;
}

void drawWorld(unsigned int shader) {
    // Note: La texture array est déjà bindée dans renderthread.c
    // Note: glClear est fait dans renderthread.c
    
    // Récupérer position caméra (thread-safe)
    pthread_mutex_lock(&game.renderThread->mutex);
    float camX = game.renderThread->cameraX;
    float camZ = game.renderThread->cameraZ;
    pthread_mutex_unlock(&game.renderThread->mutex);
    
    // Pré-calculer la visibilité des chunks pour éviter de le faire 3 fois
    // et reconstruire les meshs si nécessaire
    int chunkVisible[WORLD_CHUNKS_X][WORLD_CHUNKS_Z];
    
    for(int cx=0; cx<WORLD_CHUNKS_X; cx++) {
        for(int cz=0; cz<WORLD_CHUNKS_Z; cz++) {
            chunkVisible[cx][cz] = isChunkVisible(cx, cz, camX, camZ);
            
            if(chunkVisible[cx][cz] && game.world[cx][cz].needsRebuild) {
                rebuildChunkMesh(cx, cz);
            }
        }
    }
    
    // === PASSE 1: Dessiner les blocs OPAQUES ===
    // Active l'écriture dans le depth buffer
    glDepthMask(GL_TRUE);
    
    for(int cx=0; cx<WORLD_CHUNKS_X; cx++) {
        for(int cz=0; cz<WORLD_CHUNKS_Z; cz++) {
            if(!chunkVisible[cx][cz]) continue;
            
            Chunk *chunk = &game.world[cx][cz];
            if(chunk->vertexCount > 0) {
                glBindVertexArray(chunk->VAO);
                
                mat4 model;
                glm_mat4_identity(model);
                // Correction de l'alignement : décalage de -0.5 en X et Z pour correspondre à la physique (centrée)
                glm_translate(model, (vec3){cx * CHUNK_SIZE_X - 0.5f, 0, cz * CHUNK_SIZE_Z - 0.5f});
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, (float*)model);
                
                glDrawArrays(GL_TRIANGLES, 0, chunk->vertexCount);
            }
        }
    }
    
    // === PASSE 2: Dessiner le FEUILLAGE (fleurs) ===
    // Dessiner les fleurs AVANT le verre pour qu'elles soient masquées correctement
    // Désactive complètement le culling pour les fleurs (double-face)
    // Active le depth write pour éviter les artefacts entre fleurs
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    
    for(int cx=0; cx<WORLD_CHUNKS_X; cx++) {
        for(int cz=0; cz<WORLD_CHUNKS_Z; cz++) {
            if(!chunkVisible[cx][cz]) continue;
            
            Chunk *chunk = &game.world[cx][cz];
            if(chunk->foliageVertexCount > 0) {
                glBindVertexArray(chunk->foliageVAO);
                
                mat4 model;
                glm_mat4_identity(model);
                glm_translate(model, (vec3){cx * CHUNK_SIZE_X - 0.5f, 0, cz * CHUNK_SIZE_Z - 0.5f});
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, (float*)model);
                
                glDrawArrays(GL_TRIANGLES, 0, chunk->foliageVertexCount);
            }
        }
    }
    
    // === PASSE 2.5: Dessiner les TILE ENTITIES (Coffres, Fours, etc.) ===
    // On les dessine comme des objets opaques
    drawTileEntities(shader);
    
    glEnable(GL_CULL_FACE);
    
    // === PASSE 3: Dessiner les blocs TRANSPARENTS (verre) ===
    // Désactive l'écriture dans le depth buffer mais garde la lecture
    // Garde le culling activé pour les blocs de verre
    glDepthMask(GL_FALSE);
    
    for(int cx=0; cx<WORLD_CHUNKS_X; cx++) {
        for(int cz=0; cz<WORLD_CHUNKS_Z; cz++) {
            if(!chunkVisible[cx][cz]) continue;
            
            Chunk *chunk = &game.world[cx][cz];
            if(chunk->transparentVertexCount > 0) {
                glBindVertexArray(chunk->transparentVAO);
                
                mat4 model;
                glm_mat4_identity(model);
                glm_translate(model, (vec3){cx * CHUNK_SIZE_X - 0.5f, 0, cz * CHUNK_SIZE_Z - 0.5f});
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, (float*)model);
                
                glDrawArrays(GL_TRIANGLES, 0, chunk->transparentVertexCount);
            }
        }
    }
    
    // Réactive l'écriture dans le depth buffer
    glDepthMask(GL_TRUE);
}

void drawTileEntities(unsigned int shader) {
    // Récupérer position caméra pour le culling (optionnel, mais bon pour la perf)
    pthread_mutex_lock(&game.renderThread->mutex);
    float camX = game.renderThread->cameraX;
    float camZ = game.renderThread->cameraZ;
    pthread_mutex_unlock(&game.renderThread->mutex);
    
    // Calculer le temps une seule fois pour toutes les entités (optimisation)
    float currentTime = (float)glfwGetTime();
    
    // Stocker temporairement pour éviter appels répétés
    game.currentFrameTime = currentTime;

    for(int cx=0; cx<WORLD_CHUNKS_X; cx++) {
        for(int cz=0; cz<WORLD_CHUNKS_Z; cz++) {
            // Simple distance check (reuse logic if possible, or just simple dist)
            if(!isChunkVisible(cx, cz, camX, camZ)) continue;

            Chunk *chunk = &game.world[cx][cz];
            if(chunk->tileEntityCount == 0) continue;

            for(int i=0; i<chunk->tileEntityCount; i++) {
                TileEntity *te = &chunk->tileEntities[i];
                BlockDefinition *def = &game.blocks[te->type];

                if(def->model) {
                    // Calcul de la matrice modèle
                    mat4 model;
                    glm_mat4_identity(model);

                    // 1. Position globale du bloc
                    float globalX = (cx * CHUNK_SIZE_X) + te->x;
                    float globalY = te->y;
                    float globalZ = (cz * CHUNK_SIZE_Z) + te->z;
                    
                    glm_translate(model, (vec3){globalX, globalY, globalZ});

                    // 2. Rotation autour du centre du bloc (0.5, 0.5, 0.5)
                    // On déplace au centre, on tourne, on revient
                    glm_translate(model, (vec3){0.5f, 0.5f, 0.5f});
                    
                    // Rotation basée sur te->rotation (0=Nord, 1=Est, 2=Sud, 3=Ouest)
                    // Nord = -Z, Est = +X, Sud = +Z, Ouest = -X
                    float angle = 0.0f;
                    if(te->rotation == 1) angle = -90.0f;
                    else if(te->rotation == 2) angle = -180.0f;
                    else if(te->rotation == 3) angle = -270.0f;
                    
                    glm_rotate(model, glm_rad(angle), (vec3){0.0f, 1.0f, 0.0f});
                    
                    // 3. Revenir à la position d'origine (coin du bloc)
                    glm_translate(model, (vec3){-0.5f, -0.5f, -0.5f});
                    
                    // Utiliser la fonction de rendu spécifique si elle existe
                    if(def->renderFunc) {
                        def->renderFunc(te, def, shader, (float*)model);
                    } else {
                        // Fallback: rendu par défaut
                        renderDefaultDynamic(te, def, shader, (float*)model);
                    }
                }
            }
        }
    }
    
    // Reset l'attribut 2 à 0 pour éviter des effets de bord si d'autres shaders l'utilisent
    glVertexAttrib1f(2, 0.0f);
}

void drawCrosshair(unsigned int shader, unsigned int VAO) {
    glBindVertexArray(VAO);
    mat4 model;
    glm_mat4_identity(model);
    glUniformMatrix4fv(glGetUniformLocation(shader,"model"),1,GL_FALSE,(float*)model);
    glDrawArrays(GL_LINES, 0, 4);
}
