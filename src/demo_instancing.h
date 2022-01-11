#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

class demo_instancing : public demo
{
public:
    demo_instancing(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_instancing();
    virtual void Update(const platform_io& IO);

private:
    GL::cache& GLCache;
    GL::debug& GLDebug;

    std::vector<mat4> offsets;

    // 3d camera
    camera Camera = {};
    
    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint Texture = 0;

    GLuint instanceVBO;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;
};
