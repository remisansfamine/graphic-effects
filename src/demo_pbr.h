#pragma once
#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "opengl_helpers.h"


struct MaterialPBR
{
    GLuint normalMap;
    GLuint albedoMap;
    GLuint metallicMap;
    GLuint roughnessMap;
    GLuint aoMap;

    bool isTextured;
    bool hasNormal;

    v4 color;
    v3  albedo;
    float metallic;
    float roughness;
    float ao;

};

class demo_pbr : public demo
{

    struct sphere
    {
        GLuint MeshBuffer = 0;
        int MeshVertexCount = 0;
        vertex_descriptor MeshDesc;
    };

public:
    demo_pbr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);

    void SetupScene(GL::cache& GLCache);
    void SetupSphere(GL::cache& GLCache);

    virtual ~demo_pbr();
    virtual void Update(const platform_io& IO);

    void RenderSphere(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demos
    GLuint Program = 0;
    GLuint VAO = 0;
    MaterialPBR materialPBR;

    sphere sphere;

    bool enableSceneMultiSphere;
    float offsetZ;
    float marging;
    int sphereCount;
    float origin;

    
    std::vector<GL::light> Lights;
    GLuint LightsUniformBuffer = 0;

};

