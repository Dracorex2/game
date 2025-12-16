#ifndef OBP_LOADER_H
#define OBP_LOADER_H

#include <stdio.h>
#include <cglm/cglm.h>

// Structure pour un bone avec pivot
typedef struct {
    char name[64];
    float pivot[3];  // x, y, z
    
    // Vertices de ce bone
    float* vertices;     // Array de positions [x, y, z, x, y, z, ...]
    float* texCoords;    // Array de UVs [u, v, u, v, ...]
    unsigned int* indices; // Array d'indices pour les triangles
    
    int vertexCount;
    int texCoordCount;
    int indexCount;
    
    // OpenGL buffers (pour le rendu dynamique)
    unsigned int VAO, VBO, EBO;
} OBPBone;

// Structure pour une keyframe d'animation
typedef struct {
    float time;
    char boneName[64];
    float rotation[3];  // rx, ry, rz en degrés
} OBPKeyframe;

// Structure pour une animation
typedef struct {
    char name[64];
    float length;
    int loop;  // 0 = once, 1 = loop
    
    OBPKeyframe* keyframes;
    int keyframeCount;
} OBPAnimation;

// Structure complète du modèle OBP
typedef struct {
    OBPBone* bones;
    int boneCount;
    
    OBPAnimation* animations;
    int animationCount;
} OBPModel;

// Fonctions de chargement
OBPModel* loadOBPModel(const char* filepath);
void freeOBPModel(OBPModel* model);

// Fonction de rendu
void renderOBPModel(OBPModel* model, float time, unsigned int shader, mat4 globalModel, int blockType);

// Fonction utilitaire pour trouver un bone par nom
OBPBone* findBone(OBPModel* model, const char* name);

#endif // OBP_LOADER_H
