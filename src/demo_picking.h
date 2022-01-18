#pragma once

#include <vector>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"
#include "tavern_scene.h"

struct Model
{
    v3I ID;
    int VertexCount = 0;
    GLuint VBO = 0;
    GLuint VAO = 0;
    GLuint Texture = 0;

    v3 position;
};

class demo_picking : public demo
{
    struct picking
    {
        GLuint FBO;
        GLuint Texture;
        GLuint Program;
        int PickedID = -1;
    };

public:
    demo_picking(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_picking();
    virtual void Update(const platform_io& IO);

    void RenderOutline(const mat4& ModelViewProj);
    void RenderPickingTexture(const mat4 ViewProj);

    void DisplayDebugUI(const platform_io& IO);

private:
    GL::cache& GLCache;
    GL::debug& GLDebug;

    tavern_scene TavernScene;
    picking Picking;

    std::vector<Model> models;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;

    bool usePalette = true;
};