#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"
#include "tavern_scene.h"

// Demo of the simplest skybox
// 6 Faces + 6 Textures + Disabling depth write
class demo_shadowMap : public demo
{
    struct shadow
    {
        unsigned int width = 300;
        unsigned int height = 300;
        float superSample = 3.f;
        float aspect = 0.f;

        GLuint FBO;
        GLuint VAO;
        GLuint ID;
        GLuint Program;
    };

public:
    demo_shadowMap(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_shadowMap();
    virtual void Update(const platform_io& IO);

    void CreateShadowTexture(const platform_io& IO);
    void RenderTavern(const platform_io& IO);

    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    tavern_scene TavernScene;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO;

    // Buffer storing all the vertices
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

    shadow Shadow;
};