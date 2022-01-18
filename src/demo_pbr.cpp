#include "demo_pbr.h"

#include "color.h"
#include <imgui.h>

#include "stb_image.h"
#include <iostream>

const int LIGHT_BLOCK_BINDING_POINT = 0;

demo_pbr::demo_pbr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
	: GLDebug(GLDebug)
{
	SetupScene(GLCache);
}


void demo_pbr::SetupScene(GL::cache& GLCache)
{
    SetupSphere(GLCache);
    SetupCube(GLCache);
    SetupQuad(GLCache);

    SetupLight();

    SetupSphereMap(GLCache);
    SetupSkybox();
    SetupIrradianceMap();
    SetupPrefilterMap();
    SetupBDRF();


    //Set MultiSphereScene
    {
        enableSceneMultiSphere = false;
        offsetZ = -20.f;
        marging = 3.f;
        sphereCount = 7;
        origin = (((- marging) * (float)sphereCount) / 2.f) + marging / 2;
    }
}

void demo_pbr::SetupLight()
{
    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }

    Lights.resize(4);

    GL::light Light = {};
    Light.Enabled = true;
    Light.Diffuse = { 50,50,50 };
    Light.Specular = Light.Diffuse;
    Light.Attenuation = { 0.f, 0.f, 2.0f };

    this->Lights[0] = this->Lights[1] = this->Lights[2] = this->Lights[3] = Light;

    // Candle positions (taken from mesh data)
    this->Lights[0].Position = { -10, 10 , 5, 0.f };
    this->Lights[1].Position = { 10, 10 , 5, 0.f };
    this->Lights[2].Position = { -10, -10 , 5, 0.f };
    this->Lights[3].Position = { 10, -10 , 5, 0.f };

    // Gen light uniform buffer
    {
        glGenBuffers(1, &LightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, Lights.size() * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}


void demo_pbr::SetupSphere(GL::cache& GLCache)
{
    // Gen cube and its program
    {
        Program = GL::CreateProgramFromFiles("src/ShaderPBR.vert", "src/ShaderPBR.frag");

        {
            // Use vbo from GLCache
            sphere.MeshBuffer = GLCache.LoadObj("media/Cylinder/Cylinder.obj", 1.f, &sphere.MeshVertexCount);

            sphere.MeshDesc.Stride = sizeof(vertex_full);
            sphere.MeshDesc.HasNormal = true;
            sphere.MeshDesc.HasUV = true;
            sphere.MeshDesc.PositionOffset = OFFSETOF(vertex_full, Position);
            sphere.MeshDesc.UVOffset = OFFSETOF(vertex_full, UV);
            sphere.MeshDesc.NormalOffset = OFFSETOF(vertex_full, Normal);
            sphere.MeshDesc.TangentOffset = OFFSETOF(vertex_full, Tangent);
            sphere.MeshDesc.BitangentOffset = OFFSETOF(vertex_full, Bitangent);

            glGenVertexArrays(1, &VAO);
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, sphere.MeshBuffer);

            vertex_descriptor& Desc = sphere.MeshDesc;
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.UVOffset);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.NormalOffset);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.TangentOffset);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.BitangentOffset);

        }
    }

    //Metalic and roughness textures
    {
    
        materialPBR.albedo = { 1,1,1 };
        materialPBR.specular = 1.f;
        materialPBR.metallic = 1.f;
        materialPBR.roughness = 1.f;
        materialPBR.ao = 1.f; //Ambient occlusion
        materialPBR.hasNormal = true;
        irradiance.hasIrradianceMap = true;
    
        materialPBR.normalMap = GLCache.LoadTexture("media/PBR/RustedIron/rustediron2_normal.png", IMG_FLIP | IMG_GEN_MIPMAPS);
        materialPBR.albedoMap = GLCache.LoadTexture("media/PBR/RustedIron/rustediron2_basecolor.png", IMG_FLIP | IMG_GEN_MIPMAPS);
        materialPBR.metallicMap = GLCache.LoadTexture("media/PBR/RustedIron/rustediron2_metallic.png", IMG_FLIP | IMG_GEN_MIPMAPS);
        materialPBR.roughnessMap = GLCache.LoadTexture("media/PBR/RustedIron/rustediron2_roughness.png", IMG_FLIP | IMG_GEN_MIPMAPS);
        materialPBR.aoMap = GLCache.LoadTexture("media/PBR/baseTexture.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    }

    //No Textures
    //{
    //    materialPBR.albedo = { 1,1,1 };
    //    materialPBR.specular = 1.f;
    //    materialPBR.metallic = 1.f;
    //    materialPBR.roughness = 1.f;
    //    materialPBR.ao = 1.f; //Ambient occlusion
    //    materialPBR.hasNormal = false;
    //    irradiance.hasIrradianceMap = true;
    //
    //    materialPBR.normalMap = GLCache.LoadTexture("media/PBR/baseTexture.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.albedoMap = GLCache.LoadTexture("media/PBR/baseTexture.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.metallicMap = GLCache.LoadTexture("media/PBR/baseTexture.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.roughnessMap = GLCache.LoadTexture("media/PBR/baseTexture.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.aoMap = GLCache.LoadTexture("media/PBR/baseTexture.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    //}

    ////Anisotrope and clear coat test
    //{
    //    materialPBR.albedo = { 1,1,1 };
    //    materialPBR.specular = 1.f;
    //    materialPBR.metallic = 1.f;
    //    materialPBR.roughness = 1.f;
    //    materialPBR.ao = 1.f; //Ambient occlusion
    //    materialPBR.hasNormal = false;
    //    materialPBR.clearCoat = 0.8f;
    //    materialPBR.clearCoatRoughness = 0.f;
    //    irradiance.hasIrradianceMap = true;
    //
    //    materialPBR.albedoMap = GLCache.LoadTexture("media/PBR/Carbon/carbon_fibers_basecolor_1k.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.normalMap = GLCache.LoadTexture("media/PBR/Carbon/carbon_fibers_normal_1k.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.metallicMap = GLCache.LoadTexture("media/PBR/Carbon/baseTexture.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.roughnessMap = GLCache.LoadTexture("media/PBR/Carbon/carbon_fibers_roughness_1k.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
    //    materialPBR.aoMap = GLCache.LoadTexture("media/PBR/Carbon/baseTexture.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    //
    //}

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.normalMap"), 0);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.albedoMap"), 1);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.metallicMap"), 2);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.roughnessMap"), 3);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.aoMap"), 4);
        glUniform1i(glGetUniformLocation(Program, "irradianceMap"), 5);
        glUniform1i(glGetUniformLocation(Program, "prefilterMap"), 6);
        glUniform1i(glGetUniformLocation(Program, "brdfLUT"), 7);
    }

}

void demo_pbr::SetupCube(GL::cache& GLCache)
{
    vertex_descriptor Descriptor = {};
    Descriptor.Stride = sizeof(vertex);
    Descriptor.HasUV = true;
    Descriptor.PositionOffset = OFFSETOF(vertex, Position);
    Descriptor.UVOffset = OFFSETOF(vertex, UV);

    // Create cube vertices
    vertex Cube[36];
    cube.vertexCount = 36;
    Mesh::BuildCube(Cube, Cube + cube.vertexCount, Descriptor);

    // Upload cube to gpu (VRAM)
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cube.vertexCount * sizeof(vertex), Cube, GL_STATIC_DRAW);

    // Create a vertex array
    glGenVertexArrays(1, &cube.VAO);
    glBindVertexArray(cube.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(Descriptor.PositionOffset));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void demo_pbr::SetupQuad(GL::cache& GLCache)
{
    // Create a descriptor based on the `struct vertex` format
    vertex_descriptor Descriptor = {};
    Descriptor.Stride = sizeof(vertex);
    Descriptor.HasUV = true;
    Descriptor.PositionOffset = OFFSETOF(vertex, Position);
    Descriptor.UVOffset = OFFSETOF(vertex, UV);

    // Create a cube in RAM
    unsigned int VBO;

    vertex Quad[6];
    quad.vertexCount = 6;
    Mesh::BuildScreenQuad(Quad, Quad + quad.vertexCount, Descriptor, { 2.f,2.f });

    // Upload cube to gpu (VRAM)
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, quad.vertexCount * sizeof(vertex), Quad, GL_STATIC_DRAW);

    // Create a vertex array
    glGenVertexArrays(1, &quad.VAO);
    glBindVertexArray(quad.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, Position));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, UV));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void demo_pbr::SetupSphereMap(GL::cache& GLCache)
{
    sphereMap.Program = GL::CreateProgramFromFiles("src/SphereMapShader.vert", "src/SphereMapShader.frag");
    //Load HDR spheremap

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf("media/Arches_3k.hdr", &width, &height, &nrComponents, 0);
    if (data)
    {
        glGenTextures(1, &sphereMap.hdrTexture);
        glBindTexture(GL_TEXTURE_2D, sphereMap.hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }

    //Capture 6 face of a sphereMap to convert in cubemap
    glGenFramebuffers(1, &sphereMap.captureFBO);
    glGenRenderbuffers(1, &sphereMap.captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, sphereMap.captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sphereMap.captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sphereMap.captureRBO);

    glUseProgram(sphereMap.Program);
    glUniform1i(glGetUniformLocation(sphereMap.Program, "equirectangularMap"), 0);

}


void demo_pbr::SetupSkybox()
{
    skybox.Program = GL::CreateProgramFromFiles("src/SkyboxShader.vert", "src/SkyboxShader.frag");

    //Environment cubemap
    glGenTextures(1, &skybox.envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUseProgram(skybox.Program);
    glUniform1i(glGetUniformLocation(skybox.Program, "environmentMap"), 0);
}

void demo_pbr::SetupIrradianceMap()
{
    irradiance.Program = GL::CreateProgramFromFiles("src/SkyboxShader.vert", "src/ShaderIrradianceMap.frag");

    glGenTextures(1, &irradiance.irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance.irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0,
            GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void demo_pbr::SetupPrefilterMap()
{
    prefilterMap.Program = GL::CreateProgramFromFiles("src/SkyboxShader.vert", "src/ShaderPrefilterMap.frag");

    glGenTextures(1, &prefilterMap.prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap.prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
            prefilterMap.mipMapResolution, prefilterMap.mipMapResolution, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // to combatting visible dots artifact in the reflectance
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glUseProgram(prefilterMap.Program);
    glUniform1i(glGetUniformLocation(prefilterMap.Program, "environmentMap"), 0);
}

void demo_pbr::SetupBDRF()
{
    brdf.Program = GL::CreateProgramFromFiles("src/ShaderBRDF.vert", "src/ShaderBRDF.frag");

    glGenTextures(1, &brdf.LUTTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdf.LUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdf.resolution, brdf.resolution, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

demo_pbr::~demo_pbr()
{
    glDeleteTextures(1, &materialPBR.normalMap);
    glDeleteTextures(1, &materialPBR.albedoMap);
    glDeleteTextures(1, &materialPBR.metallicMap);
    glDeleteTextures(1, &materialPBR.roughnessMap);
    glDeleteTextures(1, &materialPBR.aoMap);
    glDeleteTextures(1, &sphereMap.hdrTexture);

    glDeleteBuffers(1, &sphere.MeshBuffer);
    glDeleteBuffers(1, &LightsUniformBuffer);

    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

void demo_pbr::SetupPBR()
{
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    mat4 captureProjectionMatrix = Mat4::Perspective(Math::ToRadians(90.f), 1.0f, 0.1f, 10.f);
    mat4 captureViews[] =
    {
       Mat4::LookAt({0.0f, 0.0f, 0.0f}, {1.0f,  0.0f,  0.0f}, v3{0.0f, -1.0f,  0.0f}),
       Mat4::LookAt({0.0f, 0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f},v3{0.0f, -1.0f,  0.0f}),
       Mat4::LookAt({0.0f, 0.0f, 0.0f}, {0.0f,  1.0f,  0.0f}, v3{0.0f,  0.0f,  1.0f}),
       Mat4::LookAt({0.0f, 0.0f, 0.0f}, {0.0f, -1.0f,  0.0f}, v3{0.0f,  0.0f, -1.0f}),
       Mat4::LookAt({0.0f, 0.0f, 0.0f}, {0.0f,  0.0f,  1.0f}, v3{0.0f, -1.0f,  0.0f}),
       Mat4::LookAt({0.0f, 0.0f, 0.0f}, {0.0f,  0.0f, -1.0f}, v3{0.0f, -1.0f,  0.0f})
    };

    //Setup spheremap
    {
        // convert HDR equirectangular environment map to cubemap equivalent
        glUseProgram(sphereMap.Program);
        glUniformMatrix4fv(glGetUniformLocation(sphereMap.Program, "uProjection"), 1, GL_FALSE, captureProjectionMatrix.e);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sphereMap.hdrTexture);

        glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, sphereMap.captureFBO);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(glGetUniformLocation(sphereMap.Program, "uView"), 1, GL_FALSE, captureViews[i].e);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, skybox.envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(cube.VAO);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.envCubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    //Setup Irradiancemap
    {
        glUseProgram(irradiance.Program);

        //irradianceShader.setInt("environmentMap", 0);
        glUniformMatrix4fv(glGetUniformLocation(sphereMap.Program, "uProjection"), 1, GL_FALSE, captureProjectionMatrix.e);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.envCubemap);

        glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, sphereMap.captureFBO);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(glGetUniformLocation(sphereMap.Program, "uView"), 1, GL_FALSE, captureViews[i].e);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance.irradianceMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glBindVertexArray(cube.VAO);
            glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    //Setup prefilterMap
    {
        glUseProgram(prefilterMap.Program);
        //prefilterShader.setInt("environmentMap", 0);
        glUniformMatrix4fv(glGetUniformLocation(prefilterMap.Program, "uProjection"), 1, GL_FALSE, captureProjectionMatrix.e);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.envCubemap);

        glBindFramebuffer(GL_FRAMEBUFFER, sphereMap.captureFBO);

        for (unsigned int mip = 0; mip < prefilterMap.maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth = 128 * std::pow(0.5, mip);
            unsigned int mipHeight = 128 * std::pow(0.5, mip);
            glBindRenderbuffer(GL_RENDERBUFFER, sphereMap.captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(prefilterMap.maxMipLevels - 1);
            glUniform1f(glGetUniformLocation(prefilterMap.Program, "roughness"), roughness);

            for (unsigned int i = 0; i < 6; ++i)
            {
                glUniformMatrix4fv(glGetUniformLocation(prefilterMap.Program, "uView"), 1, GL_FALSE, captureViews[i].e);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap.prefilterMap, mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                //RenderCube
                glBindVertexArray(cube.VAO);
                glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    //Setup BRDF
    {
        glBindFramebuffer(GL_FRAMEBUFFER, sphereMap.captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, sphereMap.captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, brdf.resolution, brdf.resolution);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf.LUTTexture, 0);

        glViewport(0, 0, brdf.resolution, brdf.resolution);

        glUseProgram(brdf.Program);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Render Quad
        glBindVertexArray(quad.VAO);
        glDrawArrays(GL_TRIANGLES, 0, quad.vertexCount);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    PBRLoaded = true;
}

void demo_pbr::Update(const platform_io& IO)
{
    if (!PBRLoaded)
        SetupPBR();

    //Render Scene
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);

    if (enableSceneMultiSphere)
    {
        for (int i = 0; i < sphereCount; i++)
        {
            for (int j = 0; j < sphereCount; j++)
            {
                mat4 ModelMatrix = Mat4::Translate({ origin + marging * i, origin + marging * j, offsetZ });

                materialPBR.roughness = ((1 / (float)sphereCount) * i);
                materialPBR.metallic = ((1 / (float)sphereCount) * j);

                // Render tavern
                RenderSphere(ProjectionMatrix, ViewMatrix, ModelMatrix);
            }
        }
    }
    else
    {
        mat4 ModelMatrix = Mat4::Translate({ 0,0,-5 });

        // Render tavern
        RenderSphere(ProjectionMatrix, ViewMatrix, ModelMatrix);
    }

    glCullFace(GL_FRONT);


    //Render Skybox
    {
        glDepthFunc(GL_LEQUAL);
        // convert HDR equirectangular environment map to cubemap equivalent
        glUseProgram(skybox.Program);
        glUniformMatrix4fv(glGetUniformLocation(sphereMap.Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
        glUniformMatrix4fv(glGetUniformLocation(sphereMap.Program, "uView"), 1, GL_FALSE, ViewMatrix.e);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.envCubemap);

        glBindVertexArray(cube.VAO);
        glDrawArrays(GL_TRIANGLES, 0, cube.vertexCount);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glCullFace(GL_BACK);

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_pbr::RenderSphere(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);

    glUniform1i(glGetUniformLocation(Program, "uMaterial.hasNormalMap"), materialPBR.hasNormal);
    glUniform1i(glGetUniformLocation(Program, "hasIrradianceMap"), irradiance.hasIrradianceMap);
    glUniform3fv(glGetUniformLocation(Program, "uMaterial.albedo"), 1, materialPBR.albedo.e);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.specular"), materialPBR.specular);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.metallic"), materialPBR.metallic);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.roughness"), materialPBR.roughness);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.ao"), materialPBR.ao);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.clearCoat"), materialPBR.clearCoat);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.clearCoatRoughness"), materialPBR.clearCoatRoughness);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, LightsUniformBuffer);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, materialPBR.normalMap);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, materialPBR.albedoMap);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, materialPBR.metallicMap);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, materialPBR.roughnessMap);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, materialPBR.aoMap);

    if (irradiance.hasIrradianceMap)
    {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance.irradianceMap);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap.prefilterMap);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, brdf.LUTTexture);
    }

    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, sphere.MeshVertexCount);

}

static bool EditLight(GL::light* Light)
{
    bool Result =
            ImGui::Checkbox("Enabled", (bool*)&Light->Enabled)
        + ImGui::SliderFloat4("Position", Light->Position.e, -4.f, 4.f)
        + ImGui::ColorEdit3("Ambient", Light->Ambient.e)
        + ImGui::ColorEdit3("Diffuse", Light->Diffuse.e)
        + ImGui::ColorEdit3("Specular", Light->Specular.e)
        + ImGui::SliderFloat("Attenuation (constant)", &Light->Attenuation.e[0], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (linear)", &Light->Attenuation.e[1], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (quadratic)", &Light->Attenuation.e[2], 0.f, 10.f);
    return Result;
}

void demo_pbr::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_base", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        
        if (ImGui::TreeNodeEx("Lights"))
        {
            for (int i = 0; i < 4; ++i)
            {
                if (ImGui::TreeNode(&Lights[i], "Light[%d]", i))
                {
                    GL::light& Light = Lights[i];
                    if (EditLight(&Light))
                    {
                        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
                        glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(GL::light), sizeof(GL::light), &Light);
                        glBindBuffer(GL_UNIFORM_BUFFER, 0);
                    }

                    // Calculate attenuation based on the light values
                    if (ImGui::TreeNode("Attenuation calculator"))
                    {
                        static float Dist = 5.f;
                        float Att = 1.f / (Light.Attenuation.e[0] + Light.Attenuation.e[1] * Dist + Light.Attenuation.e[2] * Light.Attenuation.e[2] * Dist);
                        ImGui::Text("att(d) = 1.0 / (c + ld + qdd)");
                        ImGui::SliderFloat("d", &Dist, 0.f, 20.f);
                        ImGui::Text("att(%.2f) = %.2f", Dist, Att);
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Scene Settings"))
        {
            ImGui::Checkbox("EnableSceneMultiSphere", &enableSceneMultiSphere);
            ImGui::Checkbox("hasIrradianceMap", &irradiance.hasIrradianceMap);

            if (!enableSceneMultiSphere)
            {
                if (ImGui::TreeNodeEx("Material"))
                {
                    ImGui::Checkbox("hasNormal", &materialPBR.hasNormal);
                    ImGui::ColorEdit3("Albedo", materialPBR.albedo.e);
                    ImGui::SliderFloat("Specular", &materialPBR.specular, 0.f, 1.f);
                    ImGui::SliderFloat("Metallic", &materialPBR.metallic, 0.f, 1.f);
                    ImGui::SliderFloat("Roughness", &materialPBR.roughness, 0.f, 1.f);
                    ImGui::SliderFloat("AO", &materialPBR.ao, 0.f, 1.f);
                    ImGui::SliderFloat("Clear Coat", &materialPBR.clearCoat, 0.f, 1.f);
                    ImGui::SliderFloat("Clear Coat Roughness", &materialPBR.clearCoatRoughness, 0.f, 1.f);

                    ImGui::TreePop();
                }
            }
            else
            {
                if (ImGui::InputInt("SphereCount", &sphereCount))
                {
                    origin = (((-marging) * (float)sphereCount) / 2.f) + marging / 2;
                    if (sphereCount <= 0) sphereCount = 1;
                }

                ImGui::InputFloat("OffsetZ", &offsetZ);
                
                if (ImGui::InputFloat("marging", &marging))
                    origin = (((-marging) * (float)sphereCount) / 2.f) + marging / 2;
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

