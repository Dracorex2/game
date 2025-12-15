#ifndef WORLDGEN_H
#define WORLDGEN_H

#include "chunk.h"

// Génère un monde aléatoire avec du terrain procédural
// seed: graine pour la génération (0 = utilise time())
void generateRandomWorld(Chunk **world, int seed);

// Génère un monde plat simple pour les tests
void generateFlatWorld(Chunk **world);

#endif
