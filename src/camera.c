#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cglm/cglm.h>
#include "camera.h"
#include "world.h"
#include "raycast.h"
#include "options.h"

// Variables globales caméra
float lastX = 400.0f;
float lastY = 300.0f;
int firstMouse = 1;

const float gravity = -9.8f;
const float groundLevel = 1.0f;

void processInput(GLFWwindow *window) {
    float cameraSpeed = 2.5f * game.deltaTime;
    vec3 newPos, temp, right;

    glm_vec3_cross(game.cameraFront, game.cameraUp, right);
    glm_vec3_normalize(right);
    vec3 frontXZ = {game.cameraFront[X],0,game.cameraFront[Z]};
    glm_vec3_normalize(frontXZ);

    // Déplacement W/A/S/D
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        glm_vec3_scale(frontXZ,cameraSpeed,temp);
        glm_vec3_add(game.plPos,temp,newPos);
        vec3 test = {newPos[X],game.plPos[Y],newPos[Z]};
        if(!checkCollisionAABB(test)) { 
            game.plPos[X]=newPos[X];
            game.plPos[Z]=newPos[Z];
        }
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm_vec3_scale(frontXZ,cameraSpeed,temp);
        glm_vec3_sub(game.plPos,temp,newPos);
        vec3 test = {newPos[X],game.plPos[Y],newPos[Z]};
        if(!checkCollisionAABB(test)) { game.plPos[X]=newPos[X]; game.plPos[Z]=newPos[Z]; }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm_vec3_scale(right,cameraSpeed,temp);
        glm_vec3_sub(game.plPos,temp,newPos);
        vec3 test = {newPos[X],game.plPos[Y],newPos[Z]};
        if(!checkCollisionAABB(test)) { game.plPos[X]=newPos[X]; game.plPos[Z]=newPos[Z]; }
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm_vec3_scale(right,cameraSpeed,temp);
        glm_vec3_add(game.plPos,temp,newPos);
        vec3 test = {newPos[X],game.plPos[Y],newPos[Z]};
        if(!checkCollisionAABB(test)) { game.plPos[X]=newPos[X]; game.plPos[Z]=newPos[Z]; }
    }

    // Saut
    if (glfwGetKey(window, GLFW_KEY_SPACE)==GLFW_PRESS && game.velocityY==0) game.velocityY=5.0f;

    // Gravité
    game.velocityY += gravity*game.deltaTime;
    newPos[X]=game.plPos[X]; newPos[Y]=game.plPos[Y]+game.velocityY*game.deltaTime; newPos[Z]=game.plPos[Z];
    if(!checkCollisionAABB(newPos)) game.plPos[Y]=newPos[Y]; else game.velocityY=0;
    if(game.plPos[Y]<groundLevel){ game.plPos[Y]=groundLevel; game.velocityY=0; }

    // Quitter
    if (glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_PRESS)
        glfwSetWindowShouldClose(window,1);
    
    // Options de rendu (touches F1-F4)
    static int f1Pressed = 0, f2Pressed = 0, f3Pressed = 0, f4Pressed = 0;
    
    // F1: Diminuer la distance de rendu
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed) {
        f1Pressed = 1;
        setRenderDistance(game.options.renderDistance - 2.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) f1Pressed = 0;
    
    // F2: Augmenter la distance de rendu
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2Pressed) {
        f2Pressed = 1;
        setRenderDistance(game.options.renderDistance + 2.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE) f2Pressed = 0;
    
    // F3: Toggle FPS display
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed) {
        f3Pressed = 1;
        toggleFpsDisplay(!game.options.showFps);
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) f3Pressed = 0;
    
    // F4: Afficher les options
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS && !f4Pressed) {
        f4Pressed = 1;
        printGameOptions();
    }
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_RELEASE) f4Pressed = 0;

    // Cycle blocks (Arrow Keys)
    static int leftPressed = 0, rightPressed = 0;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && !leftPressed) {
        leftPressed = 1;
        game.selectedBlockID--;
        if(game.selectedBlockID < 1) game.selectedBlockID = game.blockCount - 1;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE) leftPressed = 0;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && !rightPressed) {
        rightPressed = 1;
        game.selectedBlockID++;
        if(game.selectedBlockID >= game.blockCount) game.selectedBlockID = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE) rightPressed = 0;
}

void mouse_callback(GLFWwindow* window,double xpos,double ypos){
    if(firstMouse){ lastX=xpos; lastY=ypos; firstMouse=0; }
    float xoffset=xpos-lastX, yoffset=lastY-ypos;
    lastX=xpos; lastY=ypos;
    float sensitivity=0.1f;
    xoffset*=sensitivity; yoffset*=sensitivity;
    game.yaw+=xoffset; game.pitch+=yoffset;
    if(game.pitch>89)game.pitch=89; if(game.pitch<-89)game.pitch=-89;
    
    // Recalculer la direction en s'assurant qu'elle est bien normalisée
    game.cameraFront[X]=cos(glm_rad(game.yaw))*cos(glm_rad(game.pitch));
    game.cameraFront[Y]=sin(glm_rad(game.pitch));
    game.cameraFront[Z]=sin(glm_rad(game.yaw))*cos(glm_rad(game.pitch));
    
    // Normaliser pour être sûr
    glm_vec3_normalize(game.cameraFront);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    extern Chunk **world;
    
    if(action != GLFW_PRESS) return;
    
    // Position des yeux du joueur (exactement la même que la caméra de rendu)
    vec3 rayPos = {game.plPos[X], game.plPos[Y] + EYE_HEIGHT, game.plPos[Z]};
    
    // S'assurer que game.cameraFront est normalisé
    vec3 rayDir;
    glm_vec3_copy(game.cameraFront, rayDir);
    glm_vec3_normalize(rayDir);
    
    vec3 hitPos, placePos;
    
    printf("\n=== RAYCAST DEBUG ===\n");
    printf("Ray Origin: (%.2f, %.2f, %.2f)\n", rayPos[X], rayPos[Y], rayPos[Z]);
    printf("Ray Direction: (%.3f, %.3f, %.3f)\n", rayDir[X], rayDir[Y], rayDir[Z]);
    printf("Yaw: %.1f, Pitch: %.1f\n", game.yaw, game.pitch);
    
    if(raycastBlock(rayPos, rayDir, hitPos, placePos)) {
        int hx = (int)hitPos[X];
        int hy = (int)hitPos[Y];
        int hz = (int)hitPos[Z];
        
        printf("Hit Block: (%d, %d, %d)\n", hx, hy, hz);
        printf("Place Position: (%.1f, %.1f, %.1f)\n", placePos[X], placePos[Y], placePos[Z]);
        
        if(button == GLFW_MOUSE_BUTTON_LEFT) {
            // Détruire le bloc
            int cx = hx >= 0 ? hx / CHUNK_SIZE_X : (hx - CHUNK_SIZE_X + 1) / CHUNK_SIZE_X;
            int cz = hz >= 0 ? hz / CHUNK_SIZE_Z : (hz - CHUNK_SIZE_Z + 1) / CHUNK_SIZE_Z;
            
            if(cx >= 0 && cx < WORLD_CHUNKS_X && cz >= 0 && cz < WORLD_CHUNKS_Z && hy >= 0 && hy < CHUNK_SIZE_Y) {
                int lx = hx - cx * CHUNK_SIZE_X;
                int lz = hz - cz * CHUNK_SIZE_Z;
                if(lx < 0) lx += CHUNK_SIZE_X;
                if(lz < 0) lz += CHUNK_SIZE_Z;
                
                if(lx >= 0 && lx < CHUNK_SIZE_X && lz >= 0 && lz < CHUNK_SIZE_Z) {
                    game.world[cx][cz].blocks[lx][hy][lz].type = BLOCK_AIR;
                    game.world[cx][cz].needsRebuild = 1;
                    printf("✓ Block destroyed\n");
                }
            }
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT) {
            // Placer un bloc de pierre
            int px = (int)placePos[X];
            int py = (int)placePos[Y];
            int pz = (int)placePos[Z];
            
            if(px == hx && py == hy && pz == hz) {
                printf("✗ Can't place: same position as hit block\n");
                return;
            }
            
            vec3 testPos = {(float)px + 0.5f, (float)py, (float)pz + 0.5f};
            if(checkCollisionAABB(testPos)) {
                printf("✗ Can't place: collision with player\n");
                return;
            }
            
            if(py >= 0 && py < CHUNK_SIZE_Y) {
                int cx = px >= 0 ? px / CHUNK_SIZE_X : (px - CHUNK_SIZE_X + 1) / CHUNK_SIZE_X;
                int cz = pz >= 0 ? pz / CHUNK_SIZE_Z : (pz - CHUNK_SIZE_Z + 1) / CHUNK_SIZE_Z;
                
                if(cx >= 0 && cx < WORLD_CHUNKS_X && cz >= 0 && cz < WORLD_CHUNKS_Z) {
                    int lx = px - cx * CHUNK_SIZE_X;
                    int lz = pz - cz * CHUNK_SIZE_Z;
                    if(lx < 0) lx += CHUNK_SIZE_X;
                    if(lz < 0) lz += CHUNK_SIZE_Z;
                    
                    if(lx >= 0 && lx < CHUNK_SIZE_X && lz >= 0 && lz < CHUNK_SIZE_Z) {
                        if(game.world[cx][cz].blocks[lx][py][lz].type == BLOCK_AIR) {
                            // Use selected block
                            if(game.selectedBlockID > 0 && game.selectedBlockID < game.blockCount) {
                                game.world[cx][cz].blocks[lx][py][lz].type = game.selectedBlockID;
                                printf("✓ %s placed\n", game.blocks[game.selectedBlockID].name);
                                game.world[cx][cz].needsRebuild = 1;
                            }
                        } else {
                            printf("✗ Can't place: position not empty\n");
                        }
                    }
                }
            }
        }
    } else {
        printf("✗ No block hit\n");
    }
}

void framebuffer_size_callback(GLFWwindow* window,int width,int height){ 
    glViewport(0,0,width,height); 
}
