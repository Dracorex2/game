#ifndef OPTIONS_H
#define OPTIONS_H

#include "types.h"

// Fonctions pour modifier les options de jeu
void setRenderDistance(float distance);
void setFOV(float fov);
void toggleVSync(int enabled);
void toggleFpsDisplay(int show);

// Afficher les options actuelles
void printGameOptions();

#endif
