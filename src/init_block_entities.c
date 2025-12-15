#define _POSIX_C_SOURCE 200809L
#include <glad/glad.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "world.h"
#include "chunk.h"
#include "worldgen.h"
#include "blockparser.h"
#include "entityloader.h"

int initBlocksEntities(void) {
	// Charger les définitions de blocs depuis le fichier
	printf("Chargement des blocs depuis blocks.block...\n");
	if(parseBlocksFromFile("blocks.block") < 0) {
	    fprintf(stderr, "Erreur critique: impossible de charger blocks.block\n");
	    exit(1);
	}
	printf("Système de blocs initialisé: %d types de blocs\n", game.blockCount);

	// Charger les configurations d'entités (modèles complexes, renderers)
	printf("Chargement des entites depuis tile_entities.block...\n");
	loadEntitiesFromFile("tile_entities.block");
}
