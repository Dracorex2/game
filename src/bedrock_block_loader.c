#include "bedrock_block_loader.h"
#include "bedrock_loader.h"
#include <stdio.h>

// Wrapper simple pour charger un modèle de bloc
// Les blocs utilisent le même format que les entités mais sans animations complexes
BedrockModel* loadBedrockBlockModel(const char* filepath, int uploadToGPU) {
    BedrockModel* model = loadBedrockModel(filepath, uploadToGPU);
    
    if(model) {
        // Validation : les blocs ne devraient avoir qu'un seul bone racine
        // (pas de hiérarchies complexes comme les entités)
        if(model->boneCount > 10) {
            printf("Warning: Bloc avec %d bones (attendu <= 10). Considérez d'utiliser une entité.\n", 
                   model->boneCount);
        }
    }
    
    return model;
}
