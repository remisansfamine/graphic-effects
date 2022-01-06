#pragma once

#include "demo.h"

#include "opengl_helpers.h"
#include "opengl_headers.h"

#include "camera.h"

class demo_normal_map : public demo
{
public:
    demo_normal_map(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_normal_map();
    virtual void Update(const platform_io& IO);

    void InspectLights();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};
    
    int LIGHT_BLOCK_BINDING_POINT = 0;

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint DiffuseTexture = 0;
    GLuint NormalTexture = 0;

    // Lights buffer
    GLuint LightsUniformBuffer = 0;
    int LightCount = 8;

    // Lights data
    std::vector<GL::light> Lights;

    GLuint VAO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;
};
