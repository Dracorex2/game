#include <stdlib.h>
#include <glad/glad.h>
#include "bedrock_loader.h"

void freeBedrockModel(BedrockModel* model) {
    if(!model) return;
    
    for(int i=0; i<model->boneCount; i++) {
        if(model->bones[i].VAO) glDeleteVertexArrays(1, &model->bones[i].VAO);
        if(model->bones[i].VBO) glDeleteBuffers(1, &model->bones[i].VBO);
    }
    
    free(model->bones);
    free(model);
}
