#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "texture.h"
#include "lodepng/lodepng.h"

void createTextureAtlas() {
    // Déterminer les dimensions et compter les frames
    int texSize = 16;  // Taille par défaut
    int maxFrames = 1;
    
    // Initialiser Air (qui n'a pas de texture)
    game.blocks[0].animFrames = 1;
    
    // Première passe : Trouver la taille MAXIMALE et le nombre de frames
    for(int i = 1; i < game.blockCount; i++) {
        game.blocks[i].animFrames = 1;
        
        if(game.blocks[i].pixelData) {
            unsigned w = game.blocks[i].texWidth;
            unsigned h = game.blocks[i].texHeight;
            
            // Si cette texture est plus grande que les précédentes, on augmente la taille de l'atlas
            if((int)w > texSize) texSize = (int)w;
            
            // Détecter animation (hauteur multiple de largeur)
            if(h > w && (h % w == 0)) {
                game.blocks[i].animFrames = h / w;
                printf("  → Texture animée: %d frames (bloc %d: %s)\n", game.blocks[i].animFrames, i, game.blocks[i].name);
            }
            
            if(game.blocks[i].animFrames > maxFrames) {
                maxFrames = game.blocks[i].animFrames;
            }
        }
    }
    
    // Créer le Texture Array : [texSize x texSize x (blockCount * maxFrames)]
    int layers = game.blockCount * maxFrames;
    // Allocation pour RGBA (4 octets par pixel)
    unsigned char* atlasData = calloc(texSize * texSize * layers * 4, 1);
    
    printf("Texture Array: %dx%d (Max Resolution), %d blocs, %d frames max, %d layers\n", 
           texSize, texSize, game.blockCount, maxFrames, layers);
    fflush(stdout);
    
    // Deuxième passe : redimensionner et copier depuis la mémoire
    for(int i = 1; i < game.blockCount; i++) {
        if(!game.blocks[i].pixelData) continue;
        
        unsigned w = game.blocks[i].texWidth;
        unsigned h = game.blocks[i].texHeight;
        unsigned char* image = game.blocks[i].pixelData;
        
        int frames = game.blocks[i].animFrames;
        int srcFrameHeight = h / frames; // Hauteur d'une frame dans l'image source
        
        printf("  Bloc %d (%s): Source %dx%d -> Atlas %dx%d\n", 
               i, game.blocks[i].name, w, h, texSize, texSize);
        
        // Copier chaque frame dans sa layer
        for(int frame = 0; frame < frames; frame++) {
            int layerIndex = (i - 1) * maxFrames + frame;
            
            // Parcourir les pixels de la DESTINATION (l'atlas)
            for(int y = 0; y < texSize; y++) {
                for(int x = 0; x < texSize; x++) {
                    // Calculer le pixel correspondant dans la SOURCE (Nearest Neighbor Scaling)
                    // On mappe [0, texSize] vers [0, w]
                    int srcX = (x * w) / texSize;
                    int srcY_in_frame = (y * srcFrameHeight) / texSize;
                    
                    // Flip vertical de la source pour OpenGL
                    int srcY = frame * srcFrameHeight + (srcFrameHeight - 1 - srcY_in_frame);
                    
                    int srcIdx = (srcY * w + srcX) * 4;
                    int dstIdx = (layerIndex * texSize * texSize + y * texSize + x) * 4;
                    
                    atlasData[dstIdx + 0] = image[srcIdx + 0];
                    atlasData[dstIdx + 1] = image[srcIdx + 1];
                    atlasData[dstIdx + 2] = image[srcIdx + 2];
                    atlasData[dstIdx + 3] = image[srcIdx + 3];
                }
            }
        }
        
        // Libérer la mémoire pixelData maintenant qu'elle est copiée dans l'atlas
        free(game.blocks[i].pixelData);
        game.blocks[i].pixelData = NULL;
    }
    
    // Créer GL_TEXTURE_2D_ARRAY
    glGenTextures(1, &game.textureAtlas);
    glBindTexture(GL_TEXTURE_2D_ARRAY, game.textureAtlas);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, texSize, texSize, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasData);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    game.atlasMaxFrames = maxFrames;
    
    free(atlasData);
    printf("Texture Array créée avec succès\n");
}

void freeTextures() {
    if(game.textureAtlas != 0) {
        glDeleteTextures(1, &game.textureAtlas);
    }
    for(int i = 1; i < game.blockCount; i++) {
        if(game.blocks[i].textureID != 0) {
            glDeleteTextures(1, &game.blocks[i].textureID);
        }
    }
}
