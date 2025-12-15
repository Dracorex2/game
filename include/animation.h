#ifndef ANIMATION_H
#define ANIMATION_H

#include <cglm/cglm.h>

// Structure pour une clé d'animation (temps -> valeur)
typedef struct {
    float time;      // Temps en secondes
    vec3 value;      // Valeur (Rotation en degrés ou Position)
} Keyframe;

// Animation d'un os (une partie du modèle)
typedef struct {
    char boneName[64];
    
    Keyframe* rotationKeys;
    int rotationKeyCount;
    
    Keyframe* positionKeys;
    int positionKeyCount;
    
    // Pivot point (nécessaire car l'OBJ ne l'a pas)
    vec3 pivot; 
} BoneAnimation;

// Une animation complète
typedef struct {
    char name[64];
    float duration;
    int loop;
    
    BoneAnimation* bones;
    int boneCount;
} Animation;

// Charge une animation depuis un fichier JSON Blockbench
Animation* loadAnimation(const char* path);
void freeAnimation(Animation* anim);

// Récupère la transformation interpolée à un instant T
void getAnimatedTransform(Animation* anim, const char* boneName, float time, vec3 outPos, vec3 outRot);

#endif
