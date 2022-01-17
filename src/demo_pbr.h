#pragma once
#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "opengl_helpers.h"

struct vertex
{
    v3 Position;
    v2 UV;
};


struct MaterialPBR
{
    GLuint normalMap;
    GLuint albedoMap;
    GLuint metallicMap;
    GLuint roughnessMap;
    GLuint aoMap;

    bool hasNormal;

    v3  albedo;
    float metallic;
    float roughness;
    float ao;

    float clearCoat;
    float clearCoatRoughness;

};

class demo_pbr : public demo
{

    struct Sphere
    {
        GLuint MeshBuffer = 0;
        int MeshVertexCount = 0;
        vertex_descriptor MeshDesc;
    };

    struct SphereMap
    {
        GLuint Program;
        GLuint hdrTexture;
        GLuint captureFBO; //Frame buffer
        GLuint captureRBO; // Render buffer
    };

    struct Cube
    {
        GLuint VAO;
        int vertexCount;
    };

    struct Quad
    {
        GLuint VAO = 0;
        int vertexCount = 0;
    };

    struct Skybox
    {
        GLuint Program;
        GLuint envCubemap; //Environment cubemap
    };

    struct Irradiance
    {
        GLuint Program;
        GLuint irradianceMap;
        bool hasIrradianceMap;
    };

    struct PrefilterMap
    {
        GLuint Program;
        GLuint prefilterMap;
        const int maxMipLevels = 5;
        const int mipMapResolution = 128;
    };

    struct BDRF
    {
        GLuint Program;
        GLuint LUTTexture;
        const int resolution = 520;
    };

public:
    demo_pbr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);

    void SetupScene(GL::cache& GLCache);
    void SetupSphere(GL::cache& GLCache);
    void SetupCube(GL::cache& GLCache);
    void SetupQuad(GL::cache& GLCache);
    void SetupSphereMap(GL::cache& GLCache);
    void SetupSkybox();
    void SetupLight();
    void SetupIrradianceMap();
    void SetupPrefilterMap();
    void SetupBDRF();

    void SetupPBR();

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

    Sphere sphere;
    Cube cube;
    Quad quad;
    SphereMap sphereMap;
    Skybox skybox;
    Irradiance irradiance;
    PrefilterMap prefilterMap;
    BDRF brdf;

    bool enableSceneMultiSphere;
    float offsetZ;
    float marging;
    int sphereCount;
    float origin;

    std::vector<GL::light> Lights;
    GLuint LightsUniformBuffer = 0;

    bool PBRLoaded = false;
};

