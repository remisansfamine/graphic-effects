#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"
#include "demo_base.h"

// Demo of the simplest skybox
// 6 Faces + 6 Textures + Disabling depth write
class demo_skybox : public demo
{
    struct skybox
    {
        GLuint ID;
        GLuint VAO;
        GLuint VBO;
        GLuint Program;
        GLuint VertexCount;
    };

public:
    demo_skybox(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_skybox();
    virtual void Update(const platform_io& IO);

    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    demo_base DemoBase;

    std::array<int, 12> dimensions;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO;

    // Buffer storing all the vertices
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

    int reflect = 1;
    float refractRatio = 1.00f / 1.52f;

    // Skybox (positions in the VertexBuffer)
    int SkyboxStart = 0;
    int SkyboxCount = 0;

    skybox Skybox;

    // Textures
    GLuint SkyboxTextures[6] = {};
};