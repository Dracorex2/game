#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <cglm/cglm.h>

void initTextRenderer(int width, int height);
void renderText(const char* text, float x, float y, float scale, vec3 color);
void updateTextRendererSize(int width, int height);

#endif
