// Programme de test pour le loader OBP
#include <stdio.h>
#include "obp_loader.h"

int main() {
    printf("=== Test du loader OBP ===\n\n");
    
    // Charger le modèle example.obp (avec animations)
    OBPModel* model = loadOBPModel("example.obp");
    
    if (!model) {
        fprintf(stderr, "Erreur: impossible de charger le modèle\n");
        return 1;
    }
    
    printf("\n--- Informations du modèle ---\n");
    printf("Nombre de bones: %d\n", model->boneCount);
    printf("Nombre d'animations: %d\n\n", model->animationCount);
    
    // Afficher les bones
    for (int i = 0; i < model->boneCount; i++) {
        OBPBone* bone = &model->bones[i];
        printf("Bone %d: %s\n", i, bone->name);
        printf("  Pivot: [%.1f, %.1f, %.1f]\n", 
               bone->pivot[0], bone->pivot[1], bone->pivot[2]);
        printf("  Vertices: %d\n", bone->vertexCount);
        printf("  Texture coords: %d\n", bone->texCoordCount);
        printf("  Indices: %d\n", bone->indexCount);
        
        // Afficher les premiers vertices
        if (bone->vertexCount > 0) {
            printf("  Premiers vertices:\n");
            int maxShow = bone->vertexCount < 3 ? bone->vertexCount : 3;
            for (int v = 0; v < maxShow; v++) {
                printf("    v%d: [%.1f, %.1f, %.1f]\n", v,
                       bone->vertices[v * 3 + 0],
                       bone->vertices[v * 3 + 1],
                       bone->vertices[v * 3 + 2]);
            }
        }
        printf("\n");
    }
    
    // Afficher les animations
    if (model->animationCount > 0) {
        printf("--- Animations ---\n");
        for (int i = 0; i < model->animationCount; i++) {
            OBPAnimation* anim = &model->animations[i];
            printf("Animation %d: %s\n", i, anim->name);
            printf("  Durée: %.2f secondes\n", anim->length);
            printf("  Loop: %s\n", anim->loop ? "Oui" : "Non");
            printf("  Keyframes: %d\n", anim->keyframeCount);
            
            // Afficher les keyframes
            for (int k = 0; k < anim->keyframeCount; k++) {
                OBPKeyframe* key = &anim->keyframes[k];
                printf("    %.2fs - %s: [%.1f, %.1f, %.1f]\n",
                       key->time, key->boneName,
                       key->rotation[0], key->rotation[1], key->rotation[2]);
            }
            printf("\n");
        }
    }
    
    // Test de la fonction findBone
    printf("--- Test findBone ---\n");
    OBPBone* foundBone = findBone(model, "bb_main");
    if (foundBone) {
        printf("✓ Bone 'bb_main' trouvé avec pivot [%.1f, %.1f, %.1f]\n",
               foundBone->pivot[0], foundBone->pivot[1], foundBone->pivot[2]);
    } else {
        printf("✗ Bone 'bb_main' non trouvé\n");
    }
    
    // Libérer la mémoire
    freeOBPModel(model);
    
    printf("\n✓ Test terminé avec succès !\n");
    
    return 0;
}
