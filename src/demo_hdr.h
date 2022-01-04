#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_hdr : public demo
{
public:
    demo_hdr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_hdr();
    virtual void Update(const platform_io& IO);

    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void RenderHdrTavern(const mat4& modelViewProj);

    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;
    
    GLuint ProgramHDR = 0;

    GLuint quadVAO = 0;
    GLuint vertexBuffer = 0;
    int vertexCount = 0;

    unsigned int hdrFBO;
    unsigned int rboDepth;
    unsigned int colorBuffer;

    tavern_scene TavernScene;

    bool Wireframe = false;
};
