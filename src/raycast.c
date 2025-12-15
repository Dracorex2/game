#include <math.h>
#include "raycast.h"
#include "world.h"
#include "types.h"

int raycastBlock(vec3 pos, vec3 dir, vec3 hitPos, vec3 placePos) {
    // Algorithme DDA (Digital Differential Analyzer)
    // On avance case par case (int) mais on calcule les distances initiales avec la position précise (float)
    
    // Position actuelle dans la grille (entiers)
    int x = (int)floorf(pos[X]);
    int y = (int)floorf(pos[Y]);
    int z = (int)floorf(pos[Z]);

    // Direction du rayon
    float dx = dir[X];
    float dy = dir[Y];
    float dz = dir[Z];

    // Direction de l'incrément (1 ou -1)
    int stepX = (dx > 0) ? 1 : -1;
    int stepY = (dy > 0) ? 1 : -1;
    int stepZ = (dz > 0) ? 1 : -1;

    // Distance que le rayon doit parcourir pour traverser un bloc entier dans chaque direction
    // Si dx est 0, on met une valeur très grande pour ne jamais avancer dans cette direction
    float tDeltaX = (dx == 0) ? 1e30f : fabsf(1.0f / dx);
    float tDeltaY = (dy == 0) ? 1e30f : fabsf(1.0f / dy);
    float tDeltaZ = (dz == 0) ? 1e30f : fabsf(1.0f / dz);

    // Distance initiale jusqu'à la première frontière de bloc
    // C'est ICI qu'on utilise la position précise (float) du joueur pour initialiser le DDA
    float distToNextX = (stepX > 0) ? (floorf(pos[X]) + 1.0f - pos[X]) : (pos[X] - floorf(pos[X]));
    float tMaxX = (dx == 0) ? 1e30f : distToNextX * tDeltaX;

    float distToNextY = (stepY > 0) ? (floorf(pos[Y]) + 1.0f - pos[Y]) : (pos[Y] - floorf(pos[Y]));
    float tMaxY = (dy == 0) ? 1e30f : distToNextY * tDeltaY;

    float distToNextZ = (stepZ > 0) ? (floorf(pos[Z]) + 1.0f - pos[Z]) : (pos[Z] - floorf(pos[Z]));
    float tMaxZ = (dz == 0) ? 1e30f : distToNextZ * tDeltaZ;

    float maxDistance = 8.0f;
    float dist = 0.0f;

    // Pour savoir quel bloc on a touché en dernier (pour le placement)
    int lastX = x;
    int lastY = y;
    int lastZ = z;

    // Boucle de raycasting
    for(int i = 0; i < 100; i++) {
        // 1. Vérifier le bloc à la position de grille actuelle
        BlockType block = getBlockAt(x, y, z);
        
        if(block != BLOCK_AIR) {
            // Collision !
            hitPos[X] = (float)x;
            hitPos[Y] = (float)y;
            hitPos[Z] = (float)z;
            
            placePos[X] = (float)lastX;
            placePos[Y] = (float)lastY;
            placePos[Z] = (float)lastZ;
            return 1;
        }

        // 2. Sauvegarder la position actuelle comme "précédente"
        lastX = x;
        lastY = y;
        lastZ = z;

        // 3. Avancer au prochain voxel (logique DDA)
        if(tMaxX < tMaxY) {
            if(tMaxX < tMaxZ) {
                dist = tMaxX;
                x += stepX;
                tMaxX += tDeltaX;
            } else {
                dist = tMaxZ;
                z += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if(tMaxY < tMaxZ) {
                dist = tMaxY;
                y += stepY;
                tMaxY += tDeltaY;
            } else {
                dist = tMaxZ;
                z += stepZ;
                tMaxZ += tDeltaZ;
            }
        }

        if(dist > maxDistance) break;
    }

    return 0;
}
