#pragma once
#include <array>

#include "demo.h"

#include "opengl_helpers.h"

#include "camera.h"
#include "structures.h"


class demo_all : public demo
{
public:
    demo_all(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);

    virtual ~demo_all();
    virtual void Update(const platform_io& IO);
    void DisplayDebugUI();

private:

    void SetupLight();
    void MoveLight(const platform_io& IO, const v3& offset);

    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    GL::Texture diffuseTex  = GL::Texture(GL_TEXTURE_2D);
    GL::Texture normalMap   = GL::Texture(GL_TEXTURE_2D);

    RBS::Mesh QuadMesh;
    RBS::Mesh BackpackMesh;
    RBS::CubeMap CubeMap;

    // GL objects needed by this demos
    GL::Program UberProgram;
    
    std::vector<GL::light> Lights;
    GLuint LightsUniformBuffer = 0;
};

