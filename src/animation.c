#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "animation.h"

// Helper simple pour lire un fichier entier
static char* readFile(const char* path) {
    FILE* f = fopen(path, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* data = malloc(len + 1);
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    return data;
}

// Parsing très basique de JSON pour extraire les clés
// Note: C'est une implémentation naïve spécifique au format Blockbench pour éviter une dépendance lourde
// Format attendu: "0.0": [x, y, z]
static void parseKeyframes(const char* json, const char* keyType, Keyframe** outKeys, int* outCount) {
    // Chercher le bloc correspondant (ex: "rotation")
    char searchKey[64];
    sprintf(searchKey, "\"%s\"", keyType);
    char* start = strstr(json, searchKey);
    if(!start) {
        *outCount = 0;
        *outKeys = NULL;
        return;
    }
    
    // Compter les clés (approximatif, compte les ": [")
    int count = 0;
    char* p = strchr(start, '{'); // Début de l'objet rotation
    if(!p) return;
    char* end = p;
    int braceDepth = 1;
    while(*++end && braceDepth > 0) {
        if(*end == '{') braceDepth++;
        if(*end == '}') braceDepth--;
        if(braceDepth == 1 && *end == '[') count++;
    }
    
    if(count == 0) return;
    
    *outKeys = malloc(sizeof(Keyframe) * count);
    *outCount = count;
    
    // Parser les valeurs
    int idx = 0;
    p++; // Skip {
    while(p < end && idx < count) {
        // Trouver le temps "0.5":
        char* quote = strchr(p, '"');
        if(!quote || quote >= end) break;
        
        float time = strtof(quote + 1, NULL);
        
        // Trouver le tableau [x, y, z]
        char* bracket = strchr(quote, '[');
        if(!bracket || bracket >= end) break;
        
        float x, y, z;
        sscanf(bracket + 1, "%f, %f, %f", &x, &y, &z);
        
        (*outKeys)[idx].time = time;
        (*outKeys)[idx].value[0] = x;
        (*outKeys)[idx].value[1] = y;
        (*outKeys)[idx].value[2] = z;
        
        idx++;
        p = strchr(bracket, ']');
        if(!p) break;
        p++;
    }
}

Animation* loadAnimation(const char* path) {
    char* json = readFile(path);
    if(!json) {
        printf("Erreur: Impossible de lire le fichier d'animation %s\n", path);
        return NULL;
    }
    
    Animation* anim = calloc(1, sizeof(Animation));
    strcpy(anim->name, "imported_anim");
    anim->loop = 1; // Par défaut
    
    // Estimation de la durée max
    anim->duration = 0.0f;
    
    // Compter les os (bones)
    // On cherche "bones": { ... }
    char* bonesStart = strstr(json, "\"bones\"");
    if(bonesStart) {
        char* p = strchr(bonesStart, '{');
        if(p) {
            p++; // Skip {
            
            // Allocation initiale
            int capacity = 10;
            anim->bones = calloc(capacity, sizeof(BoneAnimation));
            anim->boneCount = 0;
            
            while(*p) {
                // Chercher le nom de l'os: "boneName": {
                char* quoteStart = strchr(p, '"');
                if(!quoteStart) break;
                
                // Vérifier si on est sorti du bloc bones (si on trouve '}' avant '"')
                char* blockEnd = strchr(p, '}');
                if(blockEnd && blockEnd < quoteStart) break;
                
                // Extraire le nom
                char boneName[64];
                char* quoteEnd = strchr(quoteStart + 1, '"');
                if(!quoteEnd) break;
                
                int nameLen = quoteEnd - (quoteStart + 1);
                if(nameLen >= 63) nameLen = 63;
                strncpy(boneName, quoteStart + 1, nameLen);
                boneName[nameLen] = 0;
                
                // Trouver le début de l'objet os: : {
                char* objStart = strchr(quoteEnd, '{');
                if(!objStart) break;
                
                // Trouver la fin de l'objet os (matching })
                int depth = 1;
                char* objEnd = objStart + 1;
                while(*objEnd && depth > 0) {
                    if(*objEnd == '{') depth++;
                    else if(*objEnd == '}') depth--;
                    objEnd++;
                }
                
                // Parser les keyframes dans ce bloc
                // On copie temporairement le bloc pour le parser (ou on passe des pointeurs bornés)
                // Ici on utilise parseKeyframes qui cherche dans la string donnée
                // On va tricher en mettant un \0 temporaire à la fin du bloc
                char save = *objEnd;
                *objEnd = 0;
                
                if(anim->boneCount >= capacity) {
                    capacity *= 2;
                    anim->bones = realloc(anim->bones, capacity * sizeof(BoneAnimation));
                }
                
                BoneAnimation* bone = &anim->bones[anim->boneCount];
                strcpy(bone->boneName, boneName);
                bone->rotationKeys = NULL;
                bone->rotationKeyCount = 0;
                
                parseKeyframes(objStart, "rotation", &bone->rotationKeys, &bone->rotationKeyCount);
                
                if(bone->rotationKeyCount > 0) {
                    printf("  Animation loaded for bone '%s': %d keys\n", boneName, bone->rotationKeyCount);
                    anim->boneCount++;
                }
                
                *objEnd = save; // Restore
                p = objEnd;
                
                // Skip comma if present
                while(*p && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t' || *p == ',')) p++;
            }
        }
    }
    
    // Calculer la durée
    for(int i=0; i<anim->boneCount; i++) {
        for(int k=0; k<anim->bones[i].rotationKeyCount; k++) {
            if(anim->bones[i].rotationKeys[k].time > anim->duration)
                anim->duration = anim->bones[i].rotationKeys[k].time;
        }
    }
    if(anim->duration == 0) anim->duration = 1.0f;
    
    free(json);
    return anim;
}

void freeAnimation(Animation* anim) {
    if(!anim) return;
    for(int i=0; i<anim->boneCount; i++) {
        if(anim->bones[i].rotationKeys) free(anim->bones[i].rotationKeys);
        if(anim->bones[i].positionKeys) free(anim->bones[i].positionKeys);
    }
    if(anim->bones) free(anim->bones);
    free(anim);
}

void getAnimatedTransform(Animation* anim, const char* boneName, float time, vec3 outPos, vec3 outRot) {
    // Valeurs par défaut
    glm_vec3_zero(outPos);
    glm_vec3_zero(outRot);
    
    if(!anim) return;
    
    // Boucler le temps
    if(anim->loop && anim->duration > 0) {
        time = fmodf(time, anim->duration);
    }
    
    // Trouver l'os
    BoneAnimation* bone = NULL;
    for(int i=0; i<anim->boneCount; i++) {
        if(strcmp(anim->bones[i].boneName, boneName) == 0) {
            bone = &anim->bones[i];
            break;
        }
    }
    
    if(!bone) return;
    
    // Interpoler Rotation
    if(bone->rotationKeyCount > 0) {
        // Trouver les clés encadrantes
        Keyframe* k1 = &bone->rotationKeys[0];
        Keyframe* k2 = &bone->rotationKeys[0];
        
        for(int i=0; i<bone->rotationKeyCount-1; i++) {
            if(time >= bone->rotationKeys[i].time && time < bone->rotationKeys[i+1].time) {
                k1 = &bone->rotationKeys[i];
                k2 = &bone->rotationKeys[i+1];
                break;
            }
        }
        
        // Interpolation linéaire
        float t = 0.0f;
        if(k2->time > k1->time) {
            t = (time - k1->time) / (k2->time - k1->time);
        }
        
        glm_vec3_lerp(k1->value, k2->value, t, outRot);
    }
    
    // Interpoler Position (similaire)
    // ... (omission pour brièveté, focus sur rotation)
}
