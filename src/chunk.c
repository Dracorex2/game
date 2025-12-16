#include <glad/glad.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "chunk.h"
#include "world.h"
#include "obp_loader.h"

// Ajoute un modèle OBP au mesh (bake la géométrie depuis les données CPU)
static inline void addOBPModel(float *vertices, int *index, int x, int y, int z, OBPModel* model, BlockType blockType, uint8_t visibleMask) {
    if(!model) return;
    
    float bType = (float)blockType;
    
    // Parcourir tous les bones du modèle OBP
    for(int b = 0; b < model->boneCount; b++) {
        OBPBone* bone = &model->bones[b];
        if(bone->indexCount == 0) continue;
        if(!bone->vertices || !bone->indices) continue;
        
        // Copier chaque triangle (via les indices)
        for(int i = 0; i < bone->indexCount; i += 3) {
            if (i + 2 >= bone->indexCount) break;
            
            unsigned int idx0 = bone->indices[i];
            unsigned int idx1 = bone->indices[i+1];
            unsigned int idx2 = bone->indices[i+2];
            
            // Vérification de sécurité
            if (idx0 >= bone->vertexCount || idx1 >= bone->vertexCount || idx2 >= bone->vertexCount) {
                continue;
            }
            
            // Calcul de la normale pour le culling
            float v0[3] = { bone->vertices[idx0*3], bone->vertices[idx0*3+1], bone->vertices[idx0*3+2] };
            float v1[3] = { bone->vertices[idx1*3], bone->vertices[idx1*3+1], bone->vertices[idx1*3+2] };
            float v2[3] = { bone->vertices[idx2*3], bone->vertices[idx2*3+1], bone->vertices[idx2*3+2] };
            
            float edge1[3] = { v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2] };
            float edge2[3] = { v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2] };
            
            float normal[3];
            normal[0] = edge1[1]*edge2[2] - edge1[2]*edge2[1];
            normal[1] = edge1[2]*edge2[0] - edge1[0]*edge2[2];
            normal[2] = edge1[0]*edge2[1] - edge1[1]*edge2[0];
            
            // Déterminer la face (0=Z+, 1=Z-, 2=X-, 3=X+, 4=Y-, 5=Y+)
            int faceDir = -1;
            float absX = (normal[0] > 0) ? normal[0] : -normal[0];
            float absY = (normal[1] > 0) ? normal[1] : -normal[1];
            float absZ = (normal[2] > 0) ? normal[2] : -normal[2];
            
            // Seuil pour considérer qu'une face est alignée (éviter les faces internes diagonales)
            if (absX > absY && absX > absZ) {
                faceDir = (normal[0] > 0) ? 3 : 2;
            } else if (absY > absX && absY > absZ) {
                faceDir = (normal[1] > 0) ? 5 : 4;
            } else if (absZ > absX && absZ > absY) {
                faceDir = (normal[2] > 0) ? 0 : 1;
            }
            
            // Si la face est alignée sur un axe, vérifier le masque de visibilité
            if (faceDir != -1) {
                if (!((visibleMask >> faceDir) & 1)) continue; // Face cachée
            }
            
            // Ajouter les 3 vertices du triangle
            unsigned int indices[3] = {idx0, idx1, idx2};
            for(int k=0; k<3; k++) {
                unsigned int idx = indices[k];
                
                // Position
                vertices[(*index)++] = (bone->vertices[idx * 3 + 0] / 16.0f) + x;
                vertices[(*index)++] = (bone->vertices[idx * 3 + 1] / 16.0f) + y;
                vertices[(*index)++] = (bone->vertices[idx * 3 + 2] / 16.0f) + z;
                
                // UV
                if (idx < bone->texCoordCount) {
                    vertices[(*index)++] = bone->texCoords[idx * 2 + 0];
                    vertices[(*index)++] = bone->texCoords[idx * 2 + 1];
                } else {
                    vertices[(*index)++] = 0.0f;
                    vertices[(*index)++] = 0.0f;
                }
                
                // Type
                vertices[(*index)++] = bType;
            }
        }
    }
}

// Vérifie si une face de cube devrait être rendue (face culling intelligent)
static inline int shouldRenderCubeFace(Chunk *chunk, int cx, int cz, int x, int y, int z, BlockType currentType, int faceDir) {
    // faceDir: 0=Z+, 1=Z-, 2=X-, 3=X+, 4=Y-, 5=Y+
    int nx = x, ny = y, nz = z;
    int ncx = cx, ncz = cz;  // Chunk voisin
    
    switch(faceDir) {
        case 0: nz++; break;  // Z+
        case 1: nz--; break;  // Z-
        case 2: nx--; break;  // X-
        case 3: nx++; break;  // X+
        case 4: ny--; break;  // Y-
        case 5: ny++; break;  // Y+
    }
    
    // Vérifier si on sort du chunk et aller dans le chunk voisin
    if(nx < 0) { ncx--; nx = CHUNK_SIZE_X - 1; }
    else if(nx >= CHUNK_SIZE_X) { ncx++; nx = 0; }
    
    if(nz < 0) { ncz--; nz = CHUNK_SIZE_Z - 1; }
    else if(nz >= CHUNK_SIZE_Z) { ncz++; nz = 0; }
    
    // Hors limites du monde = afficher
    if(ncx < 0 || ncx >= WORLD_CHUNKS_X || ncz < 0 || ncz >= WORLD_CHUNKS_Z) {
        return 1;
    }
    
    // Hors limites Y = afficher
    if(ny < 0 || ny >= CHUNK_SIZE_Y) {
        return 1;
    }
    
    // Récupérer le bloc adjacent (possiblement dans un autre chunk)
    Chunk *adjacentChunk = (ncx == cx && ncz == cz) ? chunk : &game.world[ncx][ncz];
    BlockType adjacentType = adjacentChunk->blocks[nx][ny][nz].type;
    
    // Adjacent à l'air = afficher
    if(adjacentType == BLOCK_AIR) return 1;
    
    // Sécurité
    if (adjacentType < 0 || adjacentType >= game.blockCount) return 1;
    
    // Si le bloc adjacent est opaque, ne pas afficher
    if(!game.blocks[adjacentType].transparent && !game.blocks[adjacentType].translucent) return 0;
    
    // Pour les blocs translucides : ne pas afficher les faces entre blocs identiques
    if(game.blocks[currentType].translucent && adjacentType == currentType) {
        return 0;
    }
    
    return 1;
}

// Plus besoin de addCulledCube, les blocs statiques sont rendus par le modèle directement

// Buffers partagés pour la génération de mesh (évite malloc/free à chaque chunk)
// Taille max théorique : 16*16*16 blocs * 36 vertices * 6 floats
#define MAX_CHUNK_FLOATS (CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 36 * 6)
static float* sharedOpaqueVertices = NULL;
static float* sharedTransparentVertices = NULL;
static float* sharedFoliageVertices = NULL;

// Initialise les buffers partagés si nécessaire
static void initSharedBuffers() {
    if (sharedOpaqueVertices == NULL) {
        sharedOpaqueVertices = malloc(MAX_CHUNK_FLOATS * sizeof(float));
        sharedTransparentVertices = malloc(MAX_CHUNK_FLOATS * sizeof(float));
        sharedFoliageVertices = malloc(MAX_CHUNK_FLOATS * sizeof(float));
    }
}

// Reconstruit le mesh d'un chunk avec face culling
// Sépare les blocs opaques, transparents (verre) et feuillage (fleurs) pour un rendu correct
void rebuildChunkMesh(int cx, int cz) {
    // printf("Rebuilding chunk %d, %d\n", cx, cz);
    extern Chunk **world;
    
    initSharedBuffers();
    
    Chunk *chunk = &game.world[cx][cz];
    
    // Réinitialiser les TileEntities pour ce chunk
    chunk->tileEntityCount = 0;
    
    int opaqueIndex = 0;
    int transparentIndex = 0;
    int foliageIndex = 0;
    
    // Parcourir tous les blocs du chunk
    for(int x = 0; x < CHUNK_SIZE_X; x++) {
        for(int y = 0; y < CHUNK_SIZE_Y; y++) {
            for(int z = 0; z < CHUNK_SIZE_Z; z++) {
                BlockType type = chunk->blocks[x][y][z].type;
                
                // Ignorer les blocs air
                if(type == BLOCK_AIR) continue;
                
                // Vérifier si c'est un bloc spécial (TileEntity)
                // On utilise le flag isDynamic défini dans blocks.block
                if(game.blocks[type].isDynamic) {
                    if(chunk->tileEntityCount >= chunk->tileEntityCapacity) {
                        int newCap = (chunk->tileEntityCapacity == 0) ? 4 : chunk->tileEntityCapacity * 2;
                        chunk->tileEntities = realloc(chunk->tileEntities, newCap * sizeof(TileEntity));
                        chunk->tileEntityCapacity = newCap;
                    }
                    
                    TileEntity* te = &chunk->tileEntities[chunk->tileEntityCount++];
                    te->x = x;
                    te->y = y;
                    te->z = z;
                    te->type = type;
                    te->animState = 0.0f; // Fermé par défaut
                    te->rotation = 0;     // Nord par défaut
                    
                    // NE PAS ajouter au mesh statique !
                    continue;
                }
                
                // Bloc standard : utiliser le modèle pour le baking
                
                if(!game.blocks[type].model) {
                    // Pas de modèle = on ignore (ne devrait pas arriver)
                    continue;
                }
                
                // Déterminer dans quel buffer ce bloc va
                float* vertices;
                int* index;
                
                if(game.blocks[type].translucent) {
                    vertices = sharedTransparentVertices;
                    index = &transparentIndex;
                } else if(strcmp(game.blocks[type].name, "Flower") == 0) {
                    vertices = sharedFoliageVertices;
                    index = &foliageIndex;
                } else {
                    vertices = sharedOpaqueVertices;
                    index = &opaqueIndex;
                }
                
                // Bake le modèle dans le mesh du chunk (OBP)
                if(game.blocks[type].model) {
                    // Calculer le masque de visibilité des faces
                    uint8_t visibleMask = 0;
                    
                    // Pour les fleurs et feuillages, on affiche toujours tout (pas de culling)
                    if (strcmp(game.blocks[type].name, "Flower") == 0) {
                        visibleMask = 0xFF;
                    } else {
                        if (shouldRenderCubeFace(chunk, cx, cz, x, y, z, type, 0)) visibleMask |= (1 << 0); // Z+
                        if (shouldRenderCubeFace(chunk, cx, cz, x, y, z, type, 1)) visibleMask |= (1 << 1); // Z-
                        if (shouldRenderCubeFace(chunk, cx, cz, x, y, z, type, 2)) visibleMask |= (1 << 2); // X-
                        if (shouldRenderCubeFace(chunk, cx, cz, x, y, z, type, 3)) visibleMask |= (1 << 3); // X+
                        if (shouldRenderCubeFace(chunk, cx, cz, x, y, z, type, 4)) visibleMask |= (1 << 4); // Y-
                        if (shouldRenderCubeFace(chunk, cx, cz, x, y, z, type, 5)) visibleMask |= (1 << 5); // Y+
                    }
                    
                    addOBPModel(vertices, index, x, y, z, game.blocks[type].model, type, visibleMask);
                }
            }
        }
    }
    
    // Calculer le nombre de vertices générés
    chunk->vertexCount = opaqueIndex / 6;
    chunk->transparentVertexCount = transparentIndex / 6;
    chunk->foliageVertexCount = foliageIndex / 6;
    
    // === MESH OPAQUE ===
    if(chunk->VAO == 0) {
        glGenVertexArrays(1, &chunk->VAO);
        glGenBuffers(1, &chunk->VBO);
    }
    
    glBindVertexArray(chunk->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->VBO);
    glBufferData(GL_ARRAY_BUFFER, opaqueIndex * sizeof(float), sharedOpaqueVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // === MESH TRANSPARENT ===
    if(chunk->transparentVAO == 0) {
        glGenVertexArrays(1, &chunk->transparentVAO);
        glGenBuffers(1, &chunk->transparentVBO);
    }
    
    glBindVertexArray(chunk->transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, transparentIndex * sizeof(float), sharedTransparentVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // === MESH FEUILLAGE (fleurs) ===
    if(chunk->foliageVAO == 0) {
        glGenVertexArrays(1, &chunk->foliageVAO);
        glGenBuffers(1, &chunk->foliageVBO);
    }
    
    glBindVertexArray(chunk->foliageVAO);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->foliageVBO);
    glBufferData(GL_ARRAY_BUFFER, foliageIndex * sizeof(float), sharedFoliageVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    chunk->needsRebuild = 0;
    
    // printf("Chunk (%d, %d) mesh rebuilt: %d opaque + %d transparent + %d foliage vertices\n", 
    //        cx, cz, chunk->vertexCount, chunk->transparentVertexCount, chunk->foliageVertexCount);
}

void freeChunkMesh(Chunk *chunk) {
    if(chunk->VAO != 0) {
        glDeleteVertexArrays(1, &chunk->VAO);
        glDeleteBuffers(1, &chunk->VBO);
        chunk->VAO = 0;
        chunk->VBO = 0;
    }
    if(chunk->transparentVAO != 0) {
        glDeleteVertexArrays(1, &chunk->transparentVAO);
        glDeleteBuffers(1, &chunk->transparentVBO);
        chunk->transparentVAO = 0;
        chunk->transparentVBO = 0;
    }
    if(chunk->foliageVAO != 0) {
        glDeleteVertexArrays(1, &chunk->foliageVAO);
        glDeleteBuffers(1, &chunk->foliageVBO);
        chunk->foliageVAO = 0;
        chunk->foliageVBO = 0;
    }
}
