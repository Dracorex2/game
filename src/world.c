#define _POSIX_C_SOURCE 200809L
#include <glad/glad.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "world.h"
#include "chunk.h"
#include "worldgen.h"
#include "blockparser.h"
#include "entityloader.h"

void initWorld() {
    // Charger les définitions de blocs depuis le fichier
    printf("Chargement des blocs depuis blocks.block...\n");
    if(parseBlocksFromFile("blocks.block") < 0) {
        fprintf(stderr, "Erreur critique: impossible de charger blocks.block\n");
        exit(1);
    }
    printf("Système de blocs initialisé: %d types de blocs\n", game.blockCount);
    
    // Charger les configurations d'entités (modèles complexes, renderers)
    loadEntitiesFromFile("tile_entities.block");
    
    game.world = malloc(WORLD_CHUNKS_X * sizeof(Chunk*));
    for(int cx=0; cx < WORLD_CHUNKS_X; cx++) {
        game.world[cx] = malloc(WORLD_CHUNKS_Z * sizeof(Chunk));
    }
    
    for(int cx=0; cx<WORLD_CHUNKS_X; cx++)
        for(int cz=0; cz<WORLD_CHUNKS_Z; cz++) {
            game.world[cx][cz].VAO = 0;
            game.world[cx][cz].VBO = 0;
            game.world[cx][cz].transparentVAO = 0;
            game.world[cx][cz].transparentVBO = 0;
            game.world[cx][cz].foliageVAO = 0;
            game.world[cx][cz].foliageVBO = 0;
            game.world[cx][cz].vertexCount = 0;
            game.world[cx][cz].transparentVertexCount = 0;
            game.world[cx][cz].foliageVertexCount = 0;
            game.world[cx][cz].tileEntities = NULL;
            game.world[cx][cz].tileEntityCount = 0;
            game.world[cx][cz].tileEntityCapacity = 0;
            game.world[cx][cz].needsRebuild = 1;
        }
    
    // Génération du monde
    // Utilise 0 pour un seed aléatoire basé sur le temps
    // Ou remplace par generateFlatWorld(game.world) pour un monde plat de test
    generateRandomWorld(game.world, 0);
    
    // Générer les mesh de tous les chunks
    for(int cx=0; cx<WORLD_CHUNKS_X; cx++)
        for(int cz=0; cz<WORLD_CHUNKS_Z; cz++)
            rebuildChunkMesh(cx, cz);
}

void freeWorld() {
    if(game.world) {
        for(int cx=0; cx<WORLD_CHUNKS_X; cx++) {
            for(int cz=0; cz<WORLD_CHUNKS_Z; cz++) {
                freeChunkMesh(&game.world[cx][cz]);
                if(game.world[cx][cz].tileEntities) {
                    free(game.world[cx][cz].tileEntities);
                }
            }
            free(game.world[cx]);
        }
        free(game.world);
        game.world = NULL;
    }
    
    // Libérer les chaînes allouées dynamiquement pour les blocs
    if(game.blocks) {
        for(int i = 0; i < game.blockCount; i++) {
            if(game.blocks[i].name) {
                free(game.blocks[i].name);
            }
        }
        free(game.blocks);
        game.blocks = NULL;
    }
    game.blockCount = 0;
}

BlockType getBlockAt(int worldX, int worldY, int worldZ) {
    if(worldY < 0 || worldY >= CHUNK_SIZE_Y) return BLOCK_AIR;
    
    int cx = worldX / CHUNK_SIZE_X;
    int cz = worldZ / CHUNK_SIZE_Z;
    if(worldX < 0 && worldX % CHUNK_SIZE_X != 0) cx--;
    if(worldZ < 0 && worldZ % CHUNK_SIZE_Z != 0) cz--;
    
    if(cx < 0 || cx >= WORLD_CHUNKS_X || cz < 0 || cz >= WORLD_CHUNKS_Z) return BLOCK_AIR;
    
    int lx = worldX - cx * CHUNK_SIZE_X;
    int lz = worldZ - cz * CHUNK_SIZE_Z;
    
    if(lx < 0 || lx >= CHUNK_SIZE_X || lz < 0 || lz >= CHUNK_SIZE_Z) return BLOCK_AIR;
    
    return game.world[cx][cz].blocks[lx][worldY][lz].type;
}

int checkCollisionAABB(vec3 newPos) {
    float minX = newPos[X]-CAM_WIDTH/2, maxX = newPos[X]+CAM_WIDTH/2;
    float minY = newPos[Y], maxY = newPos[Y]+CAM_HEIGHT;
    float minZ = newPos[Z]-CAM_WIDTH/2, maxZ = newPos[Z]+CAM_WIDTH/2;

    // Simplification : vérifier une grille fixe autour du joueur (3x3x4 blocs)
    // Cela couvre les pieds, le corps et la tête + marge
    int px = (int)roundf(newPos[X]);
    int py = (int)roundf(newPos[Y]);
    int pz = (int)roundf(newPos[Z]);

    for(int x = px - 1; x <= px + 1; x++) {
        for(int y = py - 1; y <= py + 2; y++) {
            for(int z = pz - 1; z <= pz + 1; z++) {
                BlockType type = getBlockAt(x, y, z);
                
                // Si c'est un bloc non solide, pas de collision
                if(!game.blocks[type].solid) 
                    continue;
                // Vérification AABB précise
                // Le bloc est centré en X/Z (x, z) mais commence à Y (y)
                // X: [x-0.5, x+0.5]
                // Y: [y, y+1.0]
                // Z: [z-0.5, z+0.5]
                float bx = (float)x;
                float by = (float)y;
                float bz = (float)z;
                
                if(maxX > bx-0.5f && minX < bx+0.5f &&
                   maxY > by && minY < by+1.0f &&
                   maxZ > bz-0.5f && minZ < bz+0.5f) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int findBlockByName(const char* name) {
    for(int i = 0; i < game.blockCount; i++) {
        if(!game.blocks[i].name) {
            fprintf(stderr, "ERREUR: game.blocks[%d].name est NULL\n", i);
            continue;
        }
        if(strcmp(game.blocks[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
