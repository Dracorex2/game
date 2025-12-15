#ifndef WORLD_H
#define WORLD_H

#include <cglm/cglm.h>
#include "types.h"

// Global world data
extern Chunk **world;
extern BlockDefinition* BLOCK_DEFINITIONS;
extern int BLOCK_COUNT;
extern unsigned int textureAtlas;

// World functions
void initWorld();
void freeWorld();
BlockType getBlockAt(int worldX, int worldY, int worldZ);
int checkCollisionAABB(vec3 newPos);

// Helper: trouve un bloc par son nom, retourne son ID ou -1 si non trouvé
int findBlockByName(const char* name);

#endif
