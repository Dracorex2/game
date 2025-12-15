#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include "bedrock_loader.h"
#include "animation.h"

// --- Helpers de parsing JSON simplifiés ---

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

static float getJsonFloat(const char* json, const char* key) {
    char search[64];
    sprintf(search, "\"%s\"", key);
    char* pos = strstr(json, search);
    if(!pos) return 0.0f;
    char* colon = strchr(pos, ':');
    if(!colon) return 0.0f;
    return strtof(colon + 1, NULL);
}

static void getJsonVec3(const char* json, const char* key, vec3 out) {
    char search[64];
    sprintf(search, "\"%s\"", key);
    char* pos = strstr(json, search);
    if(!pos) { glm_vec3_zero(out); return; }
    char* bracket = strchr(pos, '[');
    if(!bracket) { glm_vec3_zero(out); return; }
    sscanf(bracket + 1, "%f, %f, %f", &out[0], &out[1], &out[2]);
}

static void getJsonVec2(const char* json, const char* key, float* out) {
    char search[64];
    sprintf(search, "\"%s\"", key);
    char* pos = strstr(json, search);
    if(!pos) { out[0]=0; out[1]=0; return; }
    char* bracket = strchr(pos, '[');
    if(!bracket) { out[0]=0; out[1]=0; return; }
    sscanf(bracket + 1, "%f, %f", &out[0], &out[1]);
}

// --- Génération de Mesh ---

// Version robuste avec UVs explicites pour chaque coin
static void addQuadExplicit(float* buffer, int* offset,
                           vec3 tl, vec3 tr, vec3 br, vec3 bl,
                           float u_tl, float v_tl,
                           float u_tr, float v_tr,
                           float u_br, float v_br,
                           float u_bl, float v_bl) {

    // Triangle 1: BL -> BR -> TR (CCW)
    buffer[*offset + 0] = bl[0];
    buffer[*offset + 1] = bl[1];
    buffer[*offset + 2] = bl[2];
    buffer[*offset + 3] = u_bl; 
    buffer[*offset + 4] = v_bl;
    *offset += 5;

    buffer[*offset + 0] = br[0]; buffer[*offset + 1] = br[1]; buffer[*offset + 2] = br[2];
    buffer[*offset + 3] = u_br;  buffer[*offset + 4] = v_br;
    *offset += 5;

    buffer[*offset + 0] = tr[0]; buffer[*offset + 1] = tr[1]; buffer[*offset + 2] = tr[2];
    buffer[*offset + 3] = u_tr;  buffer[*offset + 4] = v_tr;
    *offset += 5;

    // Triangle 2: BL -> TR -> TL (CCW)
    buffer[*offset + 0] = bl[0]; buffer[*offset + 1] = bl[1]; buffer[*offset + 2] = bl[2];
    buffer[*offset + 3] = u_bl;  buffer[*offset + 4] = v_bl;
    *offset += 5;

    buffer[*offset + 0] = tr[0]; buffer[*offset + 1] = tr[1]; buffer[*offset + 2] = tr[2];
    buffer[*offset + 3] = u_tr;  buffer[*offset + 4] = v_tr;
    *offset += 5;

    buffer[*offset + 0] = tl[0]; buffer[*offset + 1] = tl[1]; buffer[*offset + 2] = tl[2];
    buffer[*offset + 3] = u_tl;  buffer[*offset + 4] = v_tl;
    *offset += 5;
}

static void rotatePoint(vec3 point, vec3 pivot, vec3 deg) {
    if(deg[0] == 0 && deg[1] == 0 && deg[2] == 0) return;

    vec3 temp;
    glm_vec3_sub(point, pivot, temp);

    mat4 rot = GLM_MAT4_IDENTITY_INIT;
    // Ordre de rotation X, Y, Z
    if(deg[0] != 0) glm_rotate(rot, glm_rad(-deg[0]), (vec3){1, 0, 0});
    if(deg[1] != 0) glm_rotate(rot, glm_rad(-deg[1]), (vec3){0, 1, 0});
    if(deg[2] != 0) glm_rotate(rot, glm_rad(-deg[2]), (vec3){0, 0, 1});
    
    vec4 v4 = {temp[0], temp[1], temp[2], 1.0f};
    glm_mat4_mulv(rot, v4, v4);
    
    glm_vec3_add((vec3){v4[0], v4[1], v4[2]}, pivot, point);
}

static void buildBoneMesh(BedrockBone* bone, const char* cubesJson, int texW, int texH, int isBlock) {
    int maxVerts = 20 * 36; // Allocation large
    float* vertices = malloc(maxVerts * 5 * sizeof(float));
    int vIndex = 0;
    
    char* p = (char*)cubesJson;
    while(1) {
        p = strchr(p, '{');
        if(!p) break;
        
        char* end = p;
        int depth = 1;
        while(*++end && depth > 0) {
            if(*end == '{') depth++;
            if(*end == '}') depth--;
        }
        
        char save = *end;
        *end = 0;
        
        vec3 origin, size;
        float uv[2];
        vec3 rotation = {0};
        vec3 pivot = {0};

        getJsonVec3(p, "origin", origin);
        getJsonVec3(p, "size", size);
        getJsonVec2(p, "uv", uv);
        
        // Lecture sécurisée de la rotation (doit être dans le bloc courant)
        char* rotPos = strstr(p, "\"rotation\"");
        if(rotPos && rotPos < end) {
            getJsonVec3(p, "rotation", rotation);
        }

        // Lecture du pivot : soit dans le cube, soit hérité du bone
        char* pivotPos = strstr(p, "\"pivot\"");
        if(pivotPos && pivotPos < end) {
            getJsonVec3(p, "pivot", pivot);
            glm_vec3_scale(pivot, 1.0f/16.0f, pivot); // Convertir en GL units
        } else {
            // Héritage du pivot du bone (déjà en GL units)
            glm_vec3_copy(bone->pivot, pivot);
        }
        
        *end = save;
        
        // Conversion Pixel -> OpenGL (1 bloc = 1.0f)
        float x1 = origin[0] / 16.0f;
        float y1 = origin[1] / 16.0f;
        float z1 = origin[2] / 16.0f;
        float x2 = (origin[0] + size[0]) / 16.0f;
        float y2 = (origin[1] + size[1]) / 16.0f;
        float z2 = (origin[2] + size[2]) / 16.0f;
        
        // Pivot en OpenGL units
        // glm_vec3_scale(pivot, 1.0f/16.0f, pivot); // Déjà fait plus haut

        float u = uv[0];
        float v = uv[1];
        float w = size[0];
        float h = size[1];
        float d = size[2];
        
        // Helper pour normaliser et inverser V (OpenGL (0,0) est en bas à gauche)
        // Texture (0,0) est en haut à gauche
        #define TX(val) ((val) / (float)texW)
        #define TY(val) (1.0f - ((val) / (float)texH))
        
        // Définition des coins du cube
        vec3 c000 = {x1, y1, z1};
        vec3 c100 = {x2, y1, z1};
        vec3 c010 = {x1, y2, z1};
        vec3 c110 = {x2, y2, z1};
        vec3 c001 = {x1, y1, z2};
        vec3 c101 = {x2, y1, z2};
        vec3 c011 = {x1, y2, z2};
        vec3 c111 = {x2, y2, z2};

        // Appliquer la rotation du cube
        rotatePoint(c000, pivot, rotation);
        rotatePoint(c100, pivot, rotation);
        rotatePoint(c010, pivot, rotation);
        rotatePoint(c110, pivot, rotation);
        rotatePoint(c001, pivot, rotation);
        rotatePoint(c101, pivot, rotation);
        rotatePoint(c011, pivot, rotation);
        rotatePoint(c111, pivot, rotation);

        // Si c'est un plan (fleur, vitre plate), on le traite différemment
        if (d == 0 || w == 0 || h == 0) {
             // Plan XY (Face Z)
             if (d == 0) {
                 // Front (Z-)
                 addQuadExplicit(vertices, &vIndex,
                     c010, c110, c100, c000, // TL, TR, BR, BL
                     TX(u), TY(v), TX(u+w), TY(v), TX(u+w), TY(v+h), TX(u), TY(v+h));
                 // Back (Z+)
                 addQuadExplicit(vertices, &vIndex,
                     c110, c010, c000, c100, // TL, TR, BR, BL (Inverted X for back)
                     TX(u+w), TY(v), TX(u), TY(v), TX(u), TY(v+h), TX(u+w), TY(v+h));
             }
             // Plan YZ (Face X)
             else if (w == 0) {
                 // Right (X+)
                 addQuadExplicit(vertices, &vIndex,
                     c011, c010, c000, c001, // TL, TR, BR, BL
                     TX(u), TY(v), TX(u+d), TY(v), TX(u+d), TY(v+h), TX(u), TY(v+h));
                 // Left (X-)
                 addQuadExplicit(vertices, &vIndex,
                     c010, c011, c001, c000,
                     TX(u+d), TY(v), TX(u), TY(v), TX(u), TY(v+h), TX(u+d), TY(v+h));
             }
             // Plan XZ (Face Y)
             else if (h == 0) {
                 // Top (Y+)
                 addQuadExplicit(vertices, &vIndex,
                     c001, c101, c100, c000,
                     TX(u), TY(v), TX(u+w), TY(v), TX(u+w), TY(v+d), TX(u), TY(v+d));
                 // Bottom (Y-)
                 addQuadExplicit(vertices, &vIndex,
                     c000, c100, c101, c001,
                     TX(u), TY(v+d), TX(u+w), TY(v+d), TX(u+w), TY(v), TX(u), TY(v));
             }
        } else {
            // Cube Standard - Box Mapping
            // Les UV sont lus directement depuis le JSON Bedrock
            
            // Face North (Z-): Front face
            // TL(x2, y2, z1), TR(x1, y2, z1), BR(x1, y1, z1), BL(x2, y1, z1)
            addQuadExplicit(vertices, &vIndex, c110, c010, c000, c100,
                TX(u + d + w), TY(v + d), TX(u + d), TY(v + d), TX(u + d), TY(v + d + h), TX(u + d + w), TY(v + d + h));

            // Face South (Z+): Back face
            // TL(x1, y2, z2), TR(x2, y2, z2), BR(x2, y1, z2), BL(x1, y1, z2)
            addQuadExplicit(vertices, &vIndex, c011, c111, c101, c001,
                TX(u + d + w + d), TY(v + d), TX(u + d + w + d + w), TY(v + d), TX(u + d + w + d + w), TY(v + d + h), TX(u + d + w + d), TY(v + d + h));

            // Face West (X-): Left face
            // TL(x1, y2, z1), TR(x1, y2, z2), BR(x1, y1, z2), BL(x1, y1, z1)
            addQuadExplicit(vertices, &vIndex, c010, c011, c001, c000,
                TX(u + d), TY(v + d), TX(u), TY(v + d), TX(u), TY(v + d + h), TX(u + d), TY(v + d + h));

            // Face East (X+): Right face
            // TL(x2, y2, z2), TR(x2, y2, z1), BR(x2, y1, z1), BL(x2, y1, z2)
            addQuadExplicit(vertices, &vIndex, c111, c110, c100, c101,
                TX(u + d + w + d), TY(v + d), TX(u + d + w), TY(v + d), TX(u + d + w), TY(v + d + h), TX(u + d + w + d), TY(v + d + h));

            // Face Up (Y+): Top face
            // TL(x1, y2, z1), TR(x2, y2, z1), BR(x2, y2, z2), BL(x1, y2, z2)
            addQuadExplicit(vertices, &vIndex, c010, c110, c111, c011,
                TX(u + d), TY(v), TX(u + d + w), TY(v), TX(u + d + w), TY(v + d), TX(u + d), TY(v + d));

            // Face Down (Y-): Bottom face
            // TL(x1, y1, z2), TR(x2, y1, z2), BR(x2, y1, z1), BL(x1, y1, z1)
            addQuadExplicit(vertices, &vIndex, c001, c101, c100, c000,
                TX(u + d + w), TY(v), TX(u + d + w + w), TY(v), TX(u + d + w + w), TY(v + d), TX(u + d + w), TY(v + d));
        }

        p = end + 1;
    }
    
    // Normalisation automatique pour les blocs centrés (Blockbench standard)
    if (isBlock && vIndex > 0) {
        float minX = 1000.0f, minZ = 1000.0f;
        for (int i = 0; i < vIndex; i += 5) {
            if (vertices[i] < minX) minX = vertices[i];
            if (vertices[i+2] < minZ) minZ = vertices[i+2];
        }
        
        float shiftX = 0.0f;
        float shiftZ = 0.0f;
        
        // Si le modèle commence en négatif (ex: -0.5), on le décale pour qu'il commence à 0
        if (minX < -0.1f) shiftX = 0.5f;
        if (minZ < -0.1f) shiftZ = 0.5f;
        
        if (shiftX != 0.0f || shiftZ != 0.0f) {
            for (int i = 0; i < vIndex; i += 5) {
                vertices[i] += shiftX;
                vertices[i+2] += shiftZ;
            }
        }
    }
    
    bone->vertexCount = vIndex / 5;
    
    if(bone->vertexCount > 0) {
        glGenVertexArrays(1, &bone->VAO);
        glGenBuffers(1, &bone->VBO);
        
        glBindVertexArray(bone->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, bone->VBO);
        glBufferData(GL_ARRAY_BUFFER, vIndex * sizeof(float), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
    }
    
    free(vertices);
}

BedrockModel* loadBedrockModel(const char* path, int isBlock) {
    char* json = readFile(path);
    if(!json) {
        printf("Erreur: Impossible de lire le modèle %s\n", path);
        return NULL;
    }
    
    BedrockModel* model = calloc(1, sizeof(BedrockModel));
    model->bones = calloc(64, sizeof(BedrockBone));
    
    // Utiliser les dimensions fournies si disponibles
    model->textureWidth = (int)getJsonFloat(json, "texture_width");
    model->textureHeight = (int)getJsonFloat(json, "texture_height");
    
    // Les UV sont lus directement depuis le JSON Bedrock (pas de tile_texture)

    char* bonesStart = strstr(json, "\"bones\"");
    if(!bonesStart) {
        char* geo = strstr(json, "\"minecraft:geometry\"");
        if(geo) bonesStart = strstr(geo, "\"bones\"");
    }
    
    if(bonesStart) {
        char* p = strchr(bonesStart, '[');
        if(p) {
            p++;
            while(1) {
                if(model->boneCount >= 64) break;
                p = strchr(p, '{');
                if(!p) break;
                
                char* end = p;
                int depth = 1;
                while(*++end && depth > 0) {
                    if(*end == '{') depth++;
                    if(*end == '}') depth--;
                }
                
                char save = *end;
                *end = 0;
                
                BedrockBone* bone = &model->bones[model->boneCount];
                
                char* namePos = strstr(p, "\"name\"");
                if(namePos) {
                    char* quote = strchr(namePos, ':');
                    if(quote) {
                        quote = strchr(quote, '"');
                        if(quote) sscanf(quote + 1, "%63[^\"]", bone->name);
                    }
                }
                
                char* parentPos = strstr(p, "\"parent\"");
                if(parentPos) {
                    char* quote = strchr(parentPos, ':');
                    if(quote) {
                        quote = strchr(quote, '"');
                        if(quote) sscanf(quote + 1, "%63[^\"]", bone->parentName);
                    }
                }
                
                getJsonVec3(p, "pivot", bone->pivot);
                glm_vec3_scale(bone->pivot, 1.0f/16.0f, bone->pivot);
                
                char* cubesPos = strstr(p, "\"cubes\"");
                if(cubesPos) {
                    buildBoneMesh(bone, cubesPos, model->textureWidth, model->textureHeight, isBlock);
                }
                
                model->boneCount++;
                *end = save;
                p = end + 1;
            }
        }
    }
    
    for(int i=0; i<model->boneCount; i++) {
        if(strlen(model->bones[i].parentName) > 0) {
            for(int j=0; j<model->boneCount; j++) {
                if(strcmp(model->bones[j].name, model->bones[i].parentName) == 0) {
                    model->bones[i].parent = &model->bones[j];
                    if(model->bones[j].childCount < 16) {
                        model->bones[j].children[model->bones[j].childCount++] = &model->bones[i];
                    }
                    break;
                }
            }
        }
    }
    
    free(json);
    printf("Modèle Bedrock chargé: %s (%d os)\n", path, model->boneCount);
    return model;
}

static void renderBone(BedrockBone* bone, Animation* anim, float time, unsigned int shader, mat4 parentTransform, int blockType) {
    mat4 currentTransform;
    glm_mat4_copy(parentTransform, currentTransform);
    
    glm_translate(currentTransform, bone->pivot);
    
    vec3 animRot = {0};
    vec3 animPos = {0};
    if(anim) {
        getAnimatedTransform(anim, bone->name, time, animPos, animRot);
    }
    
    if(animRot[2] != 0) glm_rotate(currentTransform, glm_rad(-animRot[2]), (vec3){0, 0, 1});
    if(animRot[1] != 0) glm_rotate(currentTransform, glm_rad(-animRot[1]), (vec3){0, 1, 0});
    if(animRot[0] != 0) glm_rotate(currentTransform, glm_rad(-animRot[0]), (vec3){1, 0, 0});
    
    glm_translate(currentTransform, (vec3){-bone->pivot[0], -bone->pivot[1], -bone->pivot[2]});
    
    if(bone->vertexCount > 0) {
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, (float*)currentTransform);
        glVertexAttrib1f(2, (float)blockType);
        glBindVertexArray(bone->VAO);
        glDrawArrays(GL_TRIANGLES, 0, bone->vertexCount);
    }
    
    for(int i=0; i<bone->childCount; i++) {
        renderBone(bone->children[i], anim, time, shader, currentTransform, blockType);
    }
}

void renderBedrockModel(BedrockModel* model, Animation* anim, float time, unsigned int shader, mat4 globalModel, int blockType) {
    if(!model) return;
    for(int i=0; i<model->boneCount; i++) {
        if(model->bones[i].parent == NULL) {
            renderBone(&model->bones[i], anim, time, shader, globalModel, blockType);
        }
    }
}
