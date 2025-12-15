#include "bedrock_entity_loader.h"
#include "bedrock_loader.h"
#include <stdio.h>

// Wrapper pour charger un modèle d'entité
// Les entités peuvent avoir des hiérarchies complexes et des animations
BedrockModel* loadBedrockEntityModel(const char* filepath, int uploadToGPU, int texW, int texH) {
    BedrockModel* model = loadBedrockModel(filepath, uploadToGPU);
    
    if(model) {
        printf("Modèle entité chargé: %s (%d bones)\n", filepath, model->boneCount);
    }
    
    return model;
}

// Charge une animation Bedrock depuis un fichier JSON
Animation* loadBedrockAnimation(const char* filepath) {
    Animation* anim = loadAnimation(filepath);
    
    if(anim) {
        printf("  @ Animation chargée: %s\n", filepath);
    }
    
    return anim;
}
