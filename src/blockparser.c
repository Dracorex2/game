#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockparser.h"
#include "bedrock_block_loader.h"
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


        snprintf(full_path, PATH_MAX, "models/%s.json", model_path);
        
        // Charger le modèle Bedrock pour BLOC STATIQUE
        game.blocks[i].model = NULL;
        
        // On passe 0,0 pour forcer l'utilisation des dimensions définies dans le JSON du modèle
        // Cela permet de respecter le mapping UV du modèle, quelle que soit la résolution réelle de la texture
        game.blocks[i].model = loadBedrockBlockModel(full_path, 1);
        if(!game.blocks[i].model) {
            fprintf(stderr, "ERREUR CRITIQUE: impossible de charger le modèle bloc %s pour %s\n", 
                    full_path, name);
            exit(1);
        }
        
        // Assigner le renderer par défaut (sera surchargé par entityloader si besoin)
        game.blocks[i].renderFunc = renderDefaultDynamic;
        
        i++;
	}
    game.blockCount = i;
    fclose(fd);
    printf("Total: %d blocs chargés depuis %s\n", game.blockCount, filepath);
}


