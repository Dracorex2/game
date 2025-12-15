#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "entities.h"
#include "renderer.h" // Pour les shaders uniforms si besoin
#include "animation.h"

void initEntitySystem() {
    // Rien de spécial pour l'instant
}

void assignEntityRenderer(BlockDefinition* def) {
    if(!def->name) return;
    
    if(strcmp(def->name, "Chest") == 0) {
        // def->renderFunc = renderChest; // DEPRECATED
        // Utiliser l'animation si disponible, sinon défaut
        if(def->animation) def->renderFunc = renderAnimated;
        else def->renderFunc = renderDefaultDynamic;
        printf("Renderer 'Chest' assigné au bloc %s (via Animation)\n", def->name);
    }
    else if(strcmp(def->name, "Hopper") == 0) {
        // Exemple: Hopper utilise le rendu par défaut pour l'instant
        def->renderFunc = renderDefaultDynamic;
    }
    else if(strcmp(def->name, "Fan") == 0 || strcmp(def->name, "Windmill") == 0) {
        def->renderFunc = renderRotator;
    }
    else if(def->animation) {
        // Si une animation est chargée, utiliser le rendu animé générique
        def->renderFunc = renderAnimated;
    }
    else {
        // Par défaut
        def->renderFunc = renderDefaultDynamic;
    }
}

// Helper pour configurer le shader
static void setupShader(unsigned int shader, mat4 model, int type) {
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, (float*)model);
    glVertexAttrib1f(2, (float)type);
}

// Rendu par défaut : dessine le modèle sans animation
void renderDefaultDynamic(TileEntity* te, BlockDefinition* def, unsigned int shader, float* modelMatrix) {
    if(!def->model) return;
    
    mat4 baseModel;
    glm_mat4_copy((vec4*)modelMatrix, baseModel);
    // Correction physique : décalage de -0.5 en X et Z
    glm_translate(baseModel, (vec3){-0.5f, 0.0f, -0.5f});
    
    renderBedrockModel(def->model, NULL, 0.0f, shader, baseModel, te->type);
}

// Rendu générique pour un objet qui tourne (ex: ventilateur, moulin)
void renderRotator(TileEntity* te, BlockDefinition* def, unsigned int shader, float* modelMatrix) {
    if(!def->model) return;
    
    mat4 baseModel;
    glm_mat4_copy((vec4*)modelMatrix, baseModel);
    
    // Rotation continue (utilise game.currentFrameTime pour éviter appels répétés)
    float angle = game.currentFrameTime * 200.0f;
    
    // Pour tourner autour du centre (0.5, 0.5, 0.5)
    // Le bloc est déjà à [0,0,0] -> [1,1,1]
    glm_translate(baseModel, (vec3){0.5f, 0.5f, 0.5f});
    glm_rotate(baseModel, glm_rad(angle), (vec3){0.0f, 1.0f, 0.0f});
    glm_translate(baseModel, (vec3){-0.5f, -0.5f, -0.5f});
    
    // Correction physique : décalage de -0.5 en X et Z
    glm_translate(baseModel, (vec3){-0.5f, 0.0f, -0.5f});
    
    renderBedrockModel(def->model, NULL, 0.0f, shader, baseModel, te->type);
}

// Rendu animé générique basé sur les données d'animation chargées
void renderAnimated(TileEntity* te, BlockDefinition* def, unsigned int shader, float* modelMatrix) {
    if(!def->model) return;
    
    mat4 baseModel;
    glm_mat4_copy((vec4*)modelMatrix, baseModel);
    // Correction physique : décalage de -0.5 en X et Z
    glm_translate(baseModel, (vec3){-0.5f, 0.0f, -0.5f});
    
    renderBedrockModel(def->model, def->animation, game.currentFrameTime, shader, baseModel, te->type);
}
