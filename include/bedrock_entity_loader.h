#ifndef BEDROCK_ENTITY_LOADER_H
#define BEDROCK_ENTITY_LOADER_H

#include "bedrock_loader.h"
#include "animation.h"

// Charge un modèle Bedrock pour une ENTITÉ/TILEENTITY
// Les entités ont des animations 3D complexes, des états, des orientations
// Supporte les animations chargées depuis des fichiers séparés
// texW/texH: dimensions de la texture (0,0 = auto-détection depuis le JSON)
BedrockModel* loadBedrockEntityModel(const char* filepath, int uploadToGPU, int texW, int texH);

// Charge une animation depuis un fichier JSON séparé
// Compatible avec le format d'animation Bedrock (keyframes, bones, rotations)
Animation* loadBedrockAnimation(const char* filepath);

#endif
