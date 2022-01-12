#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

class demo_npr : public demo
{
public:
    demo_npr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_npr();
    virtual void Update(const platform_io& IO);

    void RenderOutline();

    void DisplayDebugUI();

private:
    GL::cache& GLCache;
    GL::debug& GLDebug;

    GLuint OutlineProgram;
    GLuint OutlineFBO;
    GLuint OutlineTexture;

    GLuint QuadVAO;
    GLuint QuadFBO;

    // 3d camera
    camera Camera = {};

    v4 lightPos = { 1.f, 3.f, 1.f, 0.f };
    v3 color = { 0.6f, 0.1f, 0.2f };

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint Texture = 0;

    GLuint VAO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

    bool usePalette = true;
};