#ifndef BLOCKLOADER_H
#define BLOCKLOADER_H

#include "types.h"

// Charge les définitions de blocs depuis un fichier
// Retourne le nombre de blocs chargés (incluant AIR), ou -1 en cas d'erreur
//int loadBlocksFromFile(const char* filepath, BlockDefinition* blocks, int maxBlocks);
int parseBlocksFromFile(const char* filepath);


#endif
