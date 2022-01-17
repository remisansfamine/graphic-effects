#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_deferred_shading : public demo
{
    struct QuadMesh
    {
        vertex_full vertices[6];
        GLuint VAO = 0;
        GLuint VertexBuffer = 0; // We will store a quad (6 vertices)
        GLuint VertexCount = 6; // We will store a quad (6 vertices)
    };

    QuadMesh quad;

public:
    demo_deferred_shading(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_deferred_shading();
    virtual void Update(const platform_io& IO);

    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    GLuint geometryBuffer;
    GLuint rboDepth;

    GLuint positionTexture;
    GLuint normalTexture;
    GLuint albedoTexture;
    GLuint emissiveTexture;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint geometryProgram = 0;
    GLuint lightingProgram = 0;
    GLuint VAO = 0;

    tavern_scene TavernScene;
};
