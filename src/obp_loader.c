#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glad/glad.h>
#include "obp_loader.h"

#define MAX_LINE_LENGTH 512

// Fonction utilitaire pour sauter les espaces
static char* skipWhitespace(char* str) {
    while (*str && isspace(*str)) str++;
    return str;
}

// Fonction utilitaire pour parser une ligne commençant par un mot-clé
static int parseLine(char* line, const char* keyword, char* rest) {
    char* ptr = skipWhitespace(line);
    int len = strlen(keyword);
    
    if (strncmp(ptr, keyword, len) == 0) {
        ptr += len;
        ptr = skipWhitespace(ptr);
        strcpy(rest, ptr);
        return 1;
    }
    return 0;
}

OBPModel* loadOBPModel(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", filepath);
        return NULL;
    }
    
    OBPModel* model = (OBPModel*)calloc(1, sizeof(OBPModel));
    if (!model) {
        fclose(file);
        return NULL;
    }
    
    // Allocations temporaires dynamiques
    int boneCapacity = 16;
    model->bones = (OBPBone*)calloc(boneCapacity, sizeof(OBPBone));
    
    int animCapacity = 8;
    model->animations = (OBPAnimation*)calloc(animCapacity, sizeof(OBPAnimation));
    
    // Variables temporaires pour le parsing
    int currentBoneIndex = -1;
    int vertexCapacity = 128;
    int texCoordCapacity = 128;
    int indexCapacity = 256;
    
    // Vertices/UVs originaux du fichier OBP (réinitialisés pour chaque bone)
    float* originalVertices = NULL;
    float* originalTexCoords = NULL;
    int originalVertexCount = 0;
    int originalTexCoordCount = 0;
    
    // Vertices/UVs finaux (dupliqués selon les faces, réinitialisés pour chaque bone)
    float* tempVertices = NULL;
    float* tempTexCoords = NULL;
    unsigned int* tempIndices = NULL;
    int tempVertexCount = 0;
    int tempTexCoordCount = 0;
    int tempIndexCount = 0;
    
    int currentAnimIndex = -1;
    int keyframeCapacity = 64;
    
    char line[MAX_LINE_LENGTH];
    char rest[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        // Enlever le retour à la ligne
        line[strcspn(line, "\r\n")] = 0;
        
        // Ignorer les commentaires et lignes vides
        char* ptr = skipWhitespace(line);
        if (*ptr == '#' || *ptr == '\0') continue;
        
        // Parser 'b' (bone)
        if (parseLine(line, "b", rest)) {
            // Sauvegarder le bone précédent
            if (currentBoneIndex >= 0) {
                OBPBone* bone = &model->bones[currentBoneIndex];
                bone->vertices = tempVertices;
                bone->texCoords = tempTexCoords;
                bone->indices = tempIndices;
                bone->vertexCount = tempVertexCount;
                bone->texCoordCount = tempTexCoordCount;
                bone->indexCount = tempIndexCount;
                
                // Libérer les arrays originaux
                free(originalVertices);
                free(originalTexCoords);
            }
            
            // Nouveau bone
            if (model->boneCount >= boneCapacity) {
                boneCapacity *= 2;
                model->bones = (OBPBone*)realloc(model->bones, boneCapacity * sizeof(OBPBone));
            }
            
            currentBoneIndex = model->boneCount++;
            OBPBone* bone = &model->bones[currentBoneIndex];
            memset(bone, 0, sizeof(OBPBone));
            
            // Parser le nom et le pivot
            sscanf(rest, "%63s %f %f %f", bone->name, 
                   &bone->pivot[0], &bone->pivot[1], &bone->pivot[2]);
            
            // Réinitialiser les buffers temporaires
            originalVertices = (float*)malloc(vertexCapacity * 3 * sizeof(float));
            originalTexCoords = (float*)malloc(texCoordCapacity * 2 * sizeof(float));
            originalVertexCount = 0;
            originalTexCoordCount = 0;
            
            tempVertices = (float*)malloc(vertexCapacity * 3 * sizeof(float));
            tempTexCoords = (float*)malloc(texCoordCapacity * 2 * sizeof(float));
            tempIndices = (unsigned int*)malloc(indexCapacity * sizeof(unsigned int));
            tempVertexCount = 0;
            tempTexCoordCount = 0;
            tempIndexCount = 0;
            
            continue;
        }
        
        // Parser 'vt' (texture coordinate) AVANT 'v' car 'vt' commence par 'v'
        if (parseLine(line, "vt", rest)) {
            if (currentBoneIndex < 0) continue;
            
            if (originalTexCoordCount >= texCoordCapacity) {
                texCoordCapacity *= 2;
                originalTexCoords = (float*)realloc(originalTexCoords, texCoordCapacity * 2 * sizeof(float));
            }
            
            float u, v;
            sscanf(rest, "%f %f", &u, &v);
            originalTexCoords[originalTexCoordCount * 2 + 0] = u;
            originalTexCoords[originalTexCoordCount * 2 + 1] = v;
            originalTexCoordCount++;
            if (originalTexCoordCount <= 2) {
                // printf("DEBUG: UV parsé #%d: (%.2f, %.2f)\n", originalTexCoordCount-1, u, v);
            }
            continue;
        }
        
        // Parser 'v' (vertex) - stocker dans originaux
        if (parseLine(line, "v", rest)) {
            if (currentBoneIndex < 0) continue;
            
            if (originalVertexCount >= vertexCapacity) {
                vertexCapacity *= 2;
                originalVertices = (float*)realloc(originalVertices, vertexCapacity * 3 * sizeof(float));
            }
            
            float x, y, z;
            sscanf(rest, "%f %f %f", &x, &y, &z);
            originalVertices[originalVertexCount * 3 + 0] = x;
            originalVertices[originalVertexCount * 3 + 1] = y;
            originalVertices[originalVertexCount * 3 + 2] = z;
            originalVertexCount++;
            continue;
        }
        
        // Parser 'f' (face)
        if (parseLine(line, "f", rest)) {
            if (currentBoneIndex < 0) continue;
            
            // Parser les indices (format v/vt ou v/vt/vn)
            unsigned int v[4], vt[4], vn[4];
            int faceVertices = 0;
            
            char* token = strtok(rest, " ");
            while (token && faceVertices < 4) {
                if (sscanf(token, "%u/%u/%u", &v[faceVertices], &vt[faceVertices], &vn[faceVertices]) == 3 ||
                    sscanf(token, "%u/%u", &v[faceVertices], &vt[faceVertices]) == 2) {
                    faceVertices++;
                }
                token = strtok(NULL, " ");
            }
            
            // Pour chaque vertex de la face, créer un vertex dupliqué avec son UV
            unsigned int newIndices[4];
            for (int j = 0; j < faceVertices; j++) {
                // Ajouter nouveau vertex (duplication position + UV)
                if (tempVertexCount >= vertexCapacity) {
                    vertexCapacity *= 2;
                    tempVertices = (float*)realloc(tempVertices, vertexCapacity * 3 * sizeof(float));
                }
                if (tempTexCoordCount >= texCoordCapacity) {
                    texCoordCapacity *= 2;
                    tempTexCoords = (float*)realloc(tempTexCoords, texCoordCapacity * 2 * sizeof(float));
                }
                
                // Copier position du vertex original
                unsigned int vIdx = v[j] - 1;
                tempVertices[tempVertexCount * 3 + 0] = originalVertices[vIdx * 3 + 0];
                tempVertices[tempVertexCount * 3 + 1] = originalVertices[vIdx * 3 + 1];
                tempVertices[tempVertexCount * 3 + 2] = originalVertices[vIdx * 3 + 2];
                
                // Copier UV correspondant
                unsigned int vtIdx = vt[j] - 1;
                if (vtIdx >= originalTexCoordCount) {
                    fprintf(stderr, "ERREUR OBP: vtIdx=%u hors limites (max %d UVs originaux)\n", 
                            vtIdx, originalTexCoordCount);
                    tempTexCoords[tempTexCoordCount * 2 + 0] = 0.0f;
                    tempTexCoords[tempTexCoordCount * 2 + 1] = 0.0f;
                } else {
                    tempTexCoords[tempTexCoordCount * 2 + 0] = originalTexCoords[vtIdx * 2 + 0];
                    tempTexCoords[tempTexCoordCount * 2 + 1] = originalTexCoords[vtIdx * 2 + 1];
                }
                
                newIndices[j] = tempVertexCount;
                tempVertexCount++;
                tempTexCoordCount++;
            }
            
            // Convertir quad en triangles (inversion de l'ordre des indices pour corriger le winding)
            if (faceVertices >= 3) {
                // Triangle 1
                if (tempIndexCount + 3 >= indexCapacity) {
                    indexCapacity *= 2;
                    tempIndices = (unsigned int*)realloc(tempIndices, indexCapacity * sizeof(unsigned int));
                }
                // Inversion: 0, 2, 1 au lieu de 0, 1, 2
                tempIndices[tempIndexCount++] = newIndices[0];
                tempIndices[tempIndexCount++] = newIndices[2];
                tempIndices[tempIndexCount++] = newIndices[1];
                
                // Triangle 2 si quad
                if (faceVertices == 4) {
                    if (tempIndexCount + 3 >= indexCapacity) {
                        indexCapacity *= 2;
                        tempIndices = (unsigned int*)realloc(tempIndices, indexCapacity * sizeof(unsigned int));
                    }
                    // Inversion: 0, 3, 2 au lieu de 0, 2, 3
                    tempIndices[tempIndexCount++] = newIndices[0];
                    tempIndices[tempIndexCount++] = newIndices[3];
                    tempIndices[tempIndexCount++] = newIndices[2];
                }
            }
            continue;
        }
        
        // Parser 'anim' (animation)
        if (parseLine(line, "anim", rest)) {
            if (model->animationCount >= animCapacity) {
                animCapacity *= 2;
                model->animations = (OBPAnimation*)realloc(model->animations, animCapacity * sizeof(OBPAnimation));
            }
            
            currentAnimIndex = model->animationCount++;
            OBPAnimation* anim = &model->animations[currentAnimIndex];
            memset(anim, 0, sizeof(OBPAnimation));
            
            sscanf(rest, "%63s %f %d", anim->name, &anim->length, &anim->loop);
            
            anim->keyframes = (OBPKeyframe*)malloc(keyframeCapacity * sizeof(OBPKeyframe));
            anim->keyframeCount = 0;
            continue;
        }
        
        // Parser 'key' (keyframe)
        if (parseLine(line, "key", rest)) {
            if (currentAnimIndex < 0) continue;
            
            OBPAnimation* anim = &model->animations[currentAnimIndex];
            
            if (anim->keyframeCount >= keyframeCapacity) {
                keyframeCapacity *= 2;
                anim->keyframes = (OBPKeyframe*)realloc(anim->keyframes, keyframeCapacity * sizeof(OBPKeyframe));
            }
            
            OBPKeyframe* key = &anim->keyframes[anim->keyframeCount++];
            sscanf(rest, "%f %63s %f %f %f", &key->time, key->boneName,
                   &key->rotation[0], &key->rotation[1], &key->rotation[2]);
            continue;
        }
    }
    
    // Sauvegarder le dernier bone
    if (currentBoneIndex >= 0) {
        OBPBone* bone = &model->bones[currentBoneIndex];
        bone->vertices = tempVertices;
        bone->texCoords = tempTexCoords;
        bone->indices = tempIndices;
        bone->vertexCount = tempVertexCount;
        bone->texCoordCount = tempTexCoordCount;
        bone->indexCount = tempIndexCount;
        
        // Libérer les arrays originaux du dernier bone
        free(originalVertices);
        free(originalTexCoords);
    }
    
    fclose(file);
    
    // Générer les buffers OpenGL pour chaque bone
    for (int i = 0; i < model->boneCount; i++) {
        OBPBone* bone = &model->bones[i];
        
        if (bone->vertexCount > 0 && bone->indexCount > 0) {
            glGenVertexArrays(1, &bone->VAO);
            glGenBuffers(1, &bone->VBO);
            glGenBuffers(1, &bone->EBO);
            
            glBindVertexArray(bone->VAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, bone->VBO);
            // Format interleaved: X Y Z U V (5 floats)
            // On doit reconstruire un buffer interleaved car OBP stocke séparément
            float* interleaved = malloc(bone->vertexCount * 5 * sizeof(float));
            for(int v=0; v<bone->vertexCount; v++) {
                interleaved[v*5+0] = bone->vertices[v*3+0];
                interleaved[v*5+1] = bone->vertices[v*3+1];
                interleaved[v*5+2] = bone->vertices[v*3+2];
                
                if(v < bone->texCoordCount) {
                    interleaved[v*5+3] = bone->texCoords[v*2+0];
                    interleaved[v*5+4] = bone->texCoords[v*2+1];
                } else {
                    interleaved[v*5+3] = 0.0f;
                    interleaved[v*5+4] = 0.0f;
                }
            }
            
            glBufferData(GL_ARRAY_BUFFER, bone->vertexCount * 5 * sizeof(float), interleaved, GL_STATIC_DRAW);
            free(interleaved);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bone->EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bone->indexCount * sizeof(unsigned int), bone->indices, GL_STATIC_DRAW);
            
            // Position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            // TexCoord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            
            // BlockType attribute (instanced or uniform? Here uniform via glVertexAttrib1f in render loop)
            // Mais le shader attend un attribut vertex pour le type de bloc si on batch.
            // Pour les entités, on passe souvent le type via uniform ou attribut constant.
            // Le shader actuel utilise layout(location=2) in float aBlockType;
            // On peut utiliser glVertexAttrib1f(2, type) avant le draw call.
        }
    }
    
    printf("OBP chargé: %s (%d bones, %d animations)\n", filepath, model->boneCount, model->animationCount);
    
    return model;
}

void freeOBPModel(OBPModel* model) {
    if (!model) return;
    
    // Libérer les bones
    for (int i = 0; i < model->boneCount; i++) {
        OBPBone* bone = &model->bones[i];
        if (bone->vertices) free(bone->vertices);
        if (bone->texCoords) free(bone->texCoords);
        if (bone->indices) free(bone->indices);
        
        if (bone->VAO) glDeleteVertexArrays(1, &bone->VAO);
        if (bone->VBO) glDeleteBuffers(1, &bone->VBO);
        if (bone->EBO) glDeleteBuffers(1, &bone->EBO);
    }
    free(model->bones);
    
    // Libérer les animations
    for (int i = 0; i < model->animationCount; i++) {
        free(model->animations[i].keyframes);
    }
    free(model->animations);
    
    free(model);
}

OBPBone* findBone(OBPModel* model, const char* name) {
    if (!model || !name) return NULL;
    
    for (int i = 0; i < model->boneCount; i++) {
        if (strcmp(model->bones[i].name, name) == 0) {
            return &model->bones[i];
        }
    }
    
    return NULL;
}

// Interpolation linéaire
static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Récupérer la transformation animée pour un bone à un instant t
static void getAnimatedTransform(OBPModel* model, const char* boneName, float time, vec3 rotation) {
    // Par défaut, pas de rotation
    rotation[0] = 0; rotation[1] = 0; rotation[2] = 0;
    
    if (model->animationCount == 0) return;
    
    // On prend la première animation pour l'instant (TODO: supporter plusieurs anims)
    OBPAnimation* anim = &model->animations[0];
    
    // Gérer le bouclage
    float animTime = time;
    if (anim->loop && anim->length > 0) {
        animTime = fmod(time, anim->length);
    } else if (animTime > anim->length) {
        animTime = anim->length;
    }
    
    // Trouver les keyframes pour ce bone
    OBPKeyframe* k1 = NULL;
    OBPKeyframe* k2 = NULL;
    
    // Recherche linéaire simple (optimisable)
    for (int i = 0; i < anim->keyframeCount; i++) {
        OBPKeyframe* k = &anim->keyframes[i];
        if (strcmp(k->boneName, boneName) != 0) continue;
        
        if (k->time <= animTime) {
            if (!k1 || k->time > k1->time) k1 = k;
        }
        if (k->time >= animTime) {
            if (!k2 || k->time < k2->time) k2 = k;
        }
    }
    
    if (k1 && k2 && k1 != k2) {
        float t = (animTime - k1->time) / (k2->time - k1->time);
        rotation[0] = lerp(k1->rotation[0], k2->rotation[0], t);
        rotation[1] = lerp(k1->rotation[1], k2->rotation[1], t);
        rotation[2] = lerp(k1->rotation[2], k2->rotation[2], t);
    } else if (k1) {
        rotation[0] = k1->rotation[0];
        rotation[1] = k1->rotation[1];
        rotation[2] = k1->rotation[2];
    } else if (k2) {
        rotation[0] = k2->rotation[0];
        rotation[1] = k2->rotation[1];
        rotation[2] = k2->rotation[2];
    }
}

void renderOBPModel(OBPModel* model, float time, unsigned int shader, mat4 globalModel, int blockType) {
    if (!model) return;
    
    // Rendu de chaque bone
    for (int i = 0; i < model->boneCount; i++) {
        OBPBone* bone = &model->bones[i];
        
        if (bone->vertexCount == 0) continue;
        
        mat4 currentTransform;
        glm_mat4_copy(globalModel, currentTransform);
        
        // 1. Translation au pivot
        glm_translate(currentTransform, bone->pivot);
        
        // 2. Rotation animée
        vec3 animRot;
        getAnimatedTransform(model, bone->name, time, animRot);
        
        if(animRot[2] != 0) glm_rotate(currentTransform, glm_rad(animRot[2]), (vec3){0, 0, 1});
        if(animRot[1] != 0) glm_rotate(currentTransform, glm_rad(animRot[1]), (vec3){0, 1, 0});
        if(animRot[0] != 0) glm_rotate(currentTransform, glm_rad(animRot[0]), (vec3){1, 0, 0});
        
        // 3. Translation inverse du pivot
        glm_translate(currentTransform, (vec3){-bone->pivot[0], -bone->pivot[1], -bone->pivot[2]});
        
        // Envoi au shader
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, (float*)currentTransform);
        glVertexAttrib1f(2, (float)blockType);
        
        glBindVertexArray(bone->VAO);
        glDrawElements(GL_TRIANGLES, bone->indexCount, GL_UNSIGNED_INT, 0);
    }
}
