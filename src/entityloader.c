#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entityloader.h"
#include "types.h"
#include "world.h"
#include "obp_loader.h"
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
            // Les parties multiples ne sont plus supportées
            continue;
        } else if(line[0] == '@') {
            // Les animations sont maintenant incluses dans le fichier OBP
            continue;
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
                        freeOBPModel(game.blocks[id].model);
                        game.blocks[id].model = NULL;
                    }
                    
                    // Charger le modèle OBP
                    char fullPath[256];
                    snprintf(fullPath, 256, "%s", modelPath);
                    
                    // Remplacer l'extension .json par .obp si nécessaire
                    char* ext = strrchr(fullPath, '.');
                    if (ext && strcmp(ext, ".json") == 0) {
                        strcpy(ext, ".obp");
                    }
                    
                    game.blocks[id].model = loadOBPModel(fullPath);
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
