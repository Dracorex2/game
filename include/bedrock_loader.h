#ifndef BEDROCK_LOADER_H
#define BEDROCK_LOADER_H

#include <cglm/cglm.h>
#include "animation.h"

typedef struct BedrockCube {
    float origin[3];
    float size[3];
    float uv[2];
    int inflate;
} BedrockCube;

typedef struct BedrockBone {
    char name[64];
    char parentName[64];
    struct BedrockBone* parent; // Pointeur résolu après chargement
    
    vec3 pivot;  // Le point de pivot crucial !
    
    // Données de rendu générées
    unsigned int VAO, VBO;
    int vertexCount;
    
    // Hiérarchie
    struct BedrockBone* children[16];
    int childCount;
} BedrockBone;

typedef struct BedrockModel {
    BedrockBone* bones;
    int boneCount;
    int textureWidth;
    int textureHeight;
} BedrockModel;

// Charge un fichier .geo.json
BedrockModel* loadBedrockModel(const char* path, int isBlock);
void freeBedrockModel(BedrockModel* model);

// Rendu récursif
void renderBedrockModel(BedrockModel* model, Animation* anim, float time, unsigned int shader, mat4 globalModel, int blockType);

#endif
