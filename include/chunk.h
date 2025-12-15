#ifndef CHUNK_H
#define CHUNK_H

#include "types.h"

// Chunk mesh functions
int isBlockOpaque(BlockType type);
void rebuildChunkMesh(int cx, int cz);
void freeChunkMesh(Chunk *chunk);

#endif
