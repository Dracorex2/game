#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entityloader.h"
#include "types.h"
#include "world.h"
#include "bedrock_entity_loader.h"
#include "entities.h"

// Map string to function
static EntityRenderFunc getRendererByName(const char* name) {
    if(strcmp(name, "FanRenderer") == 0) return renderRotator;
    if(strcmp(name, "Default") == 0) return renderDefaultDynamic;
    // Add others here
    return renderDefaultDynamic;
}

void loadEntitiesFromFile(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if(!file) {
        fprintf(stderr, "Warning: could not open %s\n", filepath);
        return;
    }

    char line[256];
    int currentBlockID = -1;

    printf("Chargement des entités depuis %s...\n", filepath);

    while(fgets(line, sizeof(line), file)) {
        // Trim newline
        line[strcspn(line, "\n")] = 0;
        
        if(strlen(line) == 0 || line[0] == '#') continue;

        if(line[0] == '+') {
            // Les parties multiples ne sont plus supportées avec le système JSON
            // Toutes les parties sont définies dans le fichier JSON du modèle
            continue;
        } else if(line[0] == '@') {
            // It's an animation: @ AnimationPath
            if(currentBlockID == -1) continue;
            
            char animPath[128];
            if(sscanf(line + 1, "%s", animPath) == 1) {
                game.blocks[currentBlockID].animation = loadBedrockAnimation(animPath);
                if(game.blocks[currentBlockID].animation) {
                    printf("  @ Animation chargée: %s\n", animPath);
                    // Switch to animated renderer if we were using default
                    if(game.blocks[currentBlockID].renderFunc == renderDefaultDynamic) {
                        game.blocks[currentBlockID].renderFunc = renderAnimated;
                    }
                }
            }
        } else {
            // New Entity: BlockName RendererName ModelPath
            char blockName[64];
            char rendererName[64];
            char modelPath[128];
            
            if(sscanf(line, "%s %s %s", blockName, rendererName, modelPath) == 3) {
                int id = findBlockByName(blockName);
                if(id != -1) {
                    currentBlockID = id;
                    
                    // Override model if it was already loaded by blockloader
                    if(game.blocks[id].model) {
                        freeBedrockModel(game.blocks[id].model);
                        game.blocks[id].model = NULL;
                    }
                    
                    // Charger le modèle JSON
                    char fullPath[256];
                    // Le chemin est déjà complet dans le fichier config
                    snprintf(fullPath, 256, "%s", modelPath);
                    
                    // Note: Pour les entités, on pourrait aussi vouloir charger la texture pour avoir les dimensions
                    // Mais comme la texture est définie dans blocks.block et non ici, on utilise 0,0 pour laisser le JSON décider
                    // Ou on pourrait récupérer la texture depuis game.blocks[id].texturePath
                    
                    int texW = 0, texH = 0;
                    // TODO: Si on veut supporter le dimensionnement auto pour les entités aussi, il faudrait lire le PNG ici.
                    // Pour l'instant on laisse le JSON faire foi ou le fallback 64x64
                    
                    game.blocks[id].model = loadBedrockEntityModel(fullPath, 1, texW, texH);
                    game.blocks[id].renderFunc = getRendererByName(rendererName);
                    
                    if(game.blocks[id].model) {
                        printf("Entité configurée: %s (Model: %s)\n", blockName, fullPath);
                    } else {
                        printf("Warning: Impossible de charger le modèle %s\n", fullPath);
                    }
                } else {
                    printf("Warning: Bloc %s non trouvé pour config entité\n", blockName);
                    currentBlockID = -1;
                }
            }
        }
    }
    fclose(file);
}
