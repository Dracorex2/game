#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"
#include <cglm/cglm.h>

// Initialise le registre des entités (si besoin)
void initEntitySystem();

// Assigne la fonction de rendu appropriée à un bloc en fonction de son nom
void assignEntityRenderer(BlockDefinition* def);

// Fonctions de rendu spécifiques
void renderDefaultDynamic(TileEntity* te, BlockDefinition* def, unsigned int shader, float* modelMatrix);
void renderRotator(TileEntity* te, BlockDefinition* def, unsigned int shader, float* modelMatrix);
void renderAnimated(TileEntity* te, BlockDefinition* def, unsigned int shader, float* modelMatrix);
// Ajoutez d'autres fonctions ici (ex: renderMob, renderBoat...)

#endif
