#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockparser.h"
#include "obp_loader.h"
#include "entities.h"
#include <limits.h>
#include "lodepng/lodepng.h"

// Charge les définitions de blocs depuis un fichier
// Format: block_name id solid transparent texture_path
// Exemple: Stone 1 1 0 textures/stone.png
// Retourne le nombre total de blocs (incluant AIR)

#define S_(x) #x
#define S(x) S_(x)

int parseBlocksFromFile(const char* filepath) {
    game.blocks = malloc(sizeof(BlockDefinition));
    memset(&game.blocks[0], 0, sizeof(BlockDefinition));
    game.blocks[0].name = strdup("Air");

	FILE *fd = fopen(filepath, "r");
	if(!fd) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", filepath);
        return -1;
    }
	int i = 1;
	char *line = NULL;
	size_t len = 0;
	while (getline(&line, &len, fd) > 0) {
        if (line[0] == '#' || line[0] == '\n')
            continue;
		char name[64];
		int id;
		int solid;
		int transparant;
		int translucent;
        int isDynamic;
		char tx_path[64];
		char model_path[PATH_MAX + 1];
		
		// Lire avec ou sans model path
        // Format: Name Solid Transp Transluc Dynamic Texture Model
		int matches = sscanf(line, "%63s %i %i %i %i %"S(PATH_MAX)"s %"S(PATH_MAX)"s", 
		                     name, &solid, &transparant, &translucent, &isDynamic, tx_path, model_path);
		
		if(matches < 6) {
		    fprintf(stderr, "Erreur: ligne malformée ignorée (attendu 6+ args): %s", line);
		    continue;
		}
		
		game.blocks = realloc(game.blocks, (i + 1) * sizeof(BlockDefinition));
        game.blocks[i].name = strdup(name);
        game.blocks[i].solid = solid;
        game.blocks[i].transparent = transparant;
        game.blocks[i].translucent = translucent;
        game.blocks[i].isDynamic = isDynamic;
        game.blocks[i].animFrames = 1;  // Défaut, sera mis à jour dans createTextureAtlas()
        
        char full_path[PATH_MAX + 1];
        snprintf(full_path, PATH_MAX, "textures/%s.png", tx_path);
        
        // Pré-charger les dimensions de la texture pour le modèle
        unsigned char* pngData = NULL;
        size_t pngSize = 0;
        
        // Initialiser les champs de texture
        game.blocks[i].pixelData = NULL;
        game.blocks[i].texWidth = 0;
        game.blocks[i].texHeight = 0;
        
        // Charger le fichier en mémoire
        if(lodepng_load_file(&pngData, &pngSize, full_path) == 0) {
            // Décoder complètement l'image et la stocker
            unsigned error = lodepng_decode32(&game.blocks[i].pixelData, &game.blocks[i].texWidth, &game.blocks[i].texHeight, pngData, pngSize);
            
            if(error) {
                printf("Warning: Impossible de décoder la texture %s (Error %u)\n", tx_path, error);
            }
            
            free(pngData);
        } else {
            printf("Warning: Impossible de charger le fichier texture %s\n", tx_path);
        }


        // Déterminer le chemin du modèle OBP
        char full_path_model[PATH_MAX + 1];
        
        // Vérifier si model_path contient déjà l'extension
        if (strstr(model_path, ".obp") != NULL) {
            snprintf(full_path_model, PATH_MAX, "models/%s", model_path);
        } else if (strstr(model_path, ".json") != NULL) {
            // Remplacer .json par .obp
            char temp[PATH_MAX];
            strcpy(temp, model_path);
            char* ext = strstr(temp, ".json");
            strcpy(ext, ".obp");
            snprintf(full_path_model, PATH_MAX, "models/%s", temp);
        } else {
            // Pas d'extension, ajouter .obp
            snprintf(full_path_model, PATH_MAX, "models/%s.obp", model_path);
        }
        
        // Initialiser le pointeur
        game.blocks[i].model = NULL;
        
        // Charger le modèle OBP
        game.blocks[i].model = loadOBPModel(full_path_model);
        if(!game.blocks[i].model) {
            fprintf(stderr, "ERREUR CRITIQUE: impossible de charger le modèle OBP %s pour %s\n", 
                    full_path_model, name);
            // On continue au lieu de quitter pour permettre le debug
            // exit(1);
        } else {
            printf("Modèle OBP chargé: %s pour bloc %s\n", full_path_model, name);
        }
        
        // Assigner le renderer par défaut (sera surchargé par entityloader si besoin)
        game.blocks[i].renderFunc = renderDefaultDynamic;
        
        i++;
	}
    game.blockCount = i;
    fclose(fd);
    printf("Total: %d blocs chargés depuis %s\n", game.blockCount, filepath);
}


