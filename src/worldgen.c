#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "worldgen.h"
#include "types.h"
#include "world.h"

// Générateur de bruit de Perlin simplifié (2D)
static float noise2D(int x, int z, int seed) {
    int n = x + z * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

static float smoothNoise2D(float x, float z, int seed) {
    int intX = (int)x;
    int intZ = (int)z;
    float fracX = x - intX;
    float fracZ = z - intZ;
    
    // Interpolation bilinéaire
    float v1 = noise2D(intX, intZ, seed);
    float v2 = noise2D(intX + 1, intZ, seed);
    float v3 = noise2D(intX, intZ + 1, seed);
    float v4 = noise2D(intX + 1, intZ + 1, seed);
    
    float i1 = v1 * (1 - fracX) + v2 * fracX;
    float i2 = v3 * (1 - fracX) + v4 * fracX;
    
    return i1 * (1 - fracZ) + i2 * fracZ;
}

// Bruit de Perlin multi-octaves
static float perlinNoise2D(float x, float z, int octaves, float persistence, int seed) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for(int i = 0; i < octaves; i++) {
        total += smoothNoise2D(x * frequency, z * frequency, seed) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

// Fonction auxiliaire pour placer un arbre à une position donnée
static void placeTree(Chunk **world, int worldX, int worldY, int worldZ) {
    
    // Tronc de l'arbre (3 blocs de hauteur)
    int trunkHeight = 3;
    
    for(int y = 0; y < trunkHeight; y++) {
        int cy = (worldY + y) / CHUNK_SIZE_Y;
        if((worldY + y) >= CHUNK_SIZE_Y) continue; // Hors limites
        
        int cx = worldX / CHUNK_SIZE_X;
        int cz = worldZ / CHUNK_SIZE_Z;
        int lx = worldX % CHUNK_SIZE_X;
        int lz = worldZ % CHUNK_SIZE_Z;
        
        if(cx >= 0 && cx < WORLD_CHUNKS_X && cz >= 0 && cz < WORLD_CHUNKS_Z) {
            game.world[cx][cz].blocks[lx][worldY + y][lz].type = findBlockByName("Log");
        }
    }
    
    // Feuilles (plus petites, rayon 1-2)
    int leafY = worldY + trunkHeight;
    for(int dx = -1; dx <= 1; dx++) {
        for(int dy = -1; dy <= 1; dy++) {
            for(int dz = -1; dz <= 1; dz++) {
                // Distance approximative pour former une petite sphère
                if(abs(dx) + abs(dy) + abs(dz) <= 2) {
                    int lx = (worldX + dx) % CHUNK_SIZE_X;
                    int ly = leafY + dy;
                    int lz = (worldZ + dz) % CHUNK_SIZE_Z;
                    int cx = (worldX + dx) / CHUNK_SIZE_X;
                    int cz = (worldZ + dz) / CHUNK_SIZE_Z;
                    
                    if(cx >= 0 && cx < WORLD_CHUNKS_X && cz >= 0 && cz < WORLD_CHUNKS_Z &&
                       ly >= 0 && ly < CHUNK_SIZE_Y && lx >= 0 && lx < CHUNK_SIZE_X && 
                       lz >= 0 && lz < CHUNK_SIZE_Z) {
                        // Ne pas remplacer le tronc
                        if(game.world[cx][cz].blocks[lx][ly][lz].type == BLOCK_AIR) {
                            game.world[cx][cz].blocks[lx][ly][lz].type = findBlockByName("Leaves");
                        }
                    }
                }
            }
        }
    }
}

void generateRandomWorld(Chunk** world, int seed) {
    
    if(seed == 0) {
        seed = time(NULL);
    }
    srand(seed);
    
    printf("Génération du monde avec seed: %u\n", seed);
    
    for(int cx = 0; cx < WORLD_CHUNKS_X; cx++) {
        for(int cz = 0; cz < WORLD_CHUNKS_Z; cz++) {
            Chunk *chunk = &game.world[cx][cz];
            
            for(int x = 0; x < CHUNK_SIZE_X; x++) {
                for(int z = 0; z < CHUNK_SIZE_Z; z++) {
                    // Position mondiale
                    int worldX = cx * CHUNK_SIZE_X + x;
                    int worldZ = cz * CHUNK_SIZE_Z + z;
                    
                    // Génération de hauteur avec bruit de Perlin
                    float heightNoise = perlinNoise2D(worldX * 0.05f, worldZ * 0.05f, 4, 0.5f, seed);
                    int height = (int)(heightNoise * 6.0f + 4.0f); // Hauteur entre 0 et 10
                    
                    // Bruit pour les biomes
                    float biomeNoise = perlinNoise2D(worldX * 0.02f, worldZ * 0.02f, 2, 0.6f, seed + 1000);
                    
                    for(int y = 0; y < CHUNK_SIZE_Y; y++) {
                        BlockType blockType = BLOCK_AIR;
                        
                        if(y == 0) {
                            // Bedrock au fond
                            blockType = findBlockByName("Stone");
                        }
                        else if(y < height - 3) {
                            // Pierre en profondeur
                            blockType = findBlockByName("Stone");
                        }
                        else if(y < height - 1) {
                            // Terre sous la surface
                            blockType = findBlockByName("Dirt");
                        }
                        else if(y == height - 1) {
                            // Surface : toujours de la terre
                            blockType = findBlockByName("Grass");
                        }
                        else if(y == height) {
                            // Végétation aléatoire sur la terre
                            float vegNoise = noise2D(worldX, worldZ, seed + 2000);
                            
                            // Fleurs occasionnelles sur la surface
                            if(vegNoise > 0.65f) {
                                blockType = findBlockByName("Flower");
                            }
                        }
                        
                        chunk->blocks[x][y][z].type = blockType;
                    }
                }
            }
            
            chunk->needsRebuild = 1;
        }
    }
    // Deuxième passe : placer les arbres après avoir généré le terrain
    printf("Génération des arbres...\n");
    for(int cx = 0; cx < WORLD_CHUNKS_X; cx++) {
        for(int cz = 0; cz < WORLD_CHUNKS_Z; cz++) {
            for(int x = 0; x < CHUNK_SIZE_X; x++) {
                for(int z = 0; z < CHUNK_SIZE_Z; z++) {
                    int worldX = cx * CHUNK_SIZE_X + x;
                    int worldZ = cz * CHUNK_SIZE_Z + z;
                    
                    // Bruit pour la position des arbres
                    float treeNoise = noise2D(worldX * 2, worldZ * 2, seed + 3000);
                    
                    // Arbres plus espacés (environ 2% de chance)
                    if(treeNoise > 0.92f) {
                        // Trouver la hauteur du sol à cette position
                        float heightNoise = perlinNoise2D(worldX * 0.05f, worldZ * 0.05f, 4, 0.5f, seed);
                        int height = (int)(heightNoise * 6.0f + 4.0f);
                        
                        // Placer l'arbre sur le sol (sur le bloc de terre)
                        placeTree(world, worldX, height, worldZ);
                    }
                }
            }
        }
    }
    
    printf("Génération du monde terminée\n");
    
    // PLACEMENT DE TEST : Un bloc de test animé près du spawn
    int testID = findBlockByName("Test");
    if(testID > 0) {
        // Chunk 0,0, position locale 5,12,5
        // Assurez-vous que c'est de l'air avant (ou écrasez)
        game.world[0][0].blocks[5][12][5].type = testID;
        printf("TEST: Bloc 'Test' placé en (5, 12, 5)\n");
    } else {
        printf("TEST: Bloc 'Test' non trouvé\n");
    }

    // PLACEMENT DE TEST : Bloc de référence (TestCube)
    int testCubeID = findBlockByName("TestCube");
    if(testCubeID > 0) {
        game.world[0][0].blocks[6][12][5].type = testCubeID;
        game.world[0][0].needsRebuild = 1;  // Forcer le rebuild du chunk
        printf("TEST: Bloc 'TestCube' placé en (6, 12, 5), chunk(0,0).needsRebuild=%d\n", 
               game.world[0][0].needsRebuild);
    } else {
        printf("TEST: Bloc 'TestCube' non trouvé\n");
    }
}

void generateFlatWorld(Chunk **world) {
    printf("Génération d'un monde plat\n");
    
    for(int cx = 0; cx < WORLD_CHUNKS_X; cx++) {
        for(int cz = 0; cz < WORLD_CHUNKS_Z; cz++) {
            Chunk *chunk = &game.world[cx][cz];
            
            for(int x = 0; x < CHUNK_SIZE_X; x++) {
                for(int y = 0; y < CHUNK_SIZE_Y; y++) {
                    for(int z = 0; z < CHUNK_SIZE_Z; z++) {
                        BlockType blockType = BLOCK_AIR;
                        
                        if(y == 0) {
                            blockType = findBlockByName("Grass");
                        }
                        else if(y == 1 && (x + z) % 2 == 0) {
                            blockType = findBlockByName("Stone");
                        }
                        else if(y == 1) {
                            blockType = findBlockByName("Flower");
                        }
                        else if(y == 2 && (x + z) % 5 == 0) {
                            blockType = findBlockByName("Flower");
                        }
                        else if(y == 3 && x % 4 == 0 && z % 4 == 0) {
                            blockType = findBlockByName("Glass");
                        }
                        
                        chunk->blocks[x][y][z].type = blockType;
                    }
                }
            }
            
            chunk->needsRebuild = 1;
        }
    }
    
    printf("Monde plat généré\n");
}
