#ifndef BEDROCK_BLOCK_LOADER_H
#define BEDROCK_BLOCK_LOADER_H

#include "bedrock_loader.h"

// Charge un modèle Bedrock pour un BLOC STATIQUE
// Les blocs n'ont pas d'animations 3D complexes, juste des modèles géométriques
// texW/texH: dimensions de la texture (0,0 = auto-détection depuis le JSON)
BedrockModel* loadBedrockBlockModel(const char* filepath, int uploadToGPU);

#endif
