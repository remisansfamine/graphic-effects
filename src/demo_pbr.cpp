#include "demo_pbr.h"

#include "color.h"
#include <imgui.h>


const int LIGHT_BLOCK_BINDING_POINT = 0;

demo_pbr::demo_pbr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
	: GLDebug(GLDebug)
{
	SetupScene(GLCache);
}


void demo_pbr::SetupScene(GL::cache& GLCache)
{
    SetupSphere(GLCache);

    Lights.resize(4);

    GL::light Light = {};
    Light.Enabled = true;
    Light.Diffuse = {50,50,50};
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

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }

    //Set MultiSphereScene
    {
        enableSceneMultiSphere = false;
        offsetZ = -20.f;
        marging = 3.f;
        sphereCount = 7;
        origin = (((- marging) * (float)sphereCount) / 2.f) + marging / 2;
    }
}


void demo_pbr::SetupSphere(GL::cache& GLCache)
{
    // Gen cube and its program
    {
        Program = GL::CreateProgramFromFiles("src/ShaderPBR.vert", "src/ShaderPBR.frag");

        {
            // Use vbo from GLCache
            sphere.MeshBuffer = GLCache.LoadObj("media/Sphere/Sphere.obj", 1.f, &sphere.MeshVertexCount);

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

    materialPBR.isTextured = true;

    materialPBR.color = { 1,1,1,1 };
    materialPBR.albedo = {1,0,0};

    materialPBR.metallic = 1.f;
    materialPBR.roughness = 0.1f;
    materialPBR.ao = 0; //Ambient occlusion

    materialPBR.normalMap = GLCache.LoadTexture("media/Sphere/RustedIron/rustediron2_normal.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    materialPBR.albedoMap = GLCache.LoadTexture("media/Sphere/RustedIron/rustediron2_basecolor.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    materialPBR.metallicMap = GLCache.LoadTexture("media/Sphere/RustedIron/rustediron2_metallic.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    materialPBR.roughnessMap = GLCache.LoadTexture("media/Sphere/RustedIron/rustediron2_roughness.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    materialPBR.aoMap = 0;

    //materialPBR.normalMap = GLCache.LoadTexture("media/brickwall_normal.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
    //materialPBR.albedoMap = GLCache.LoadTexture("media/brickwall.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
    //materialPBR.metallicMap = 0;
    //materialPBR.roughnessMap = 0;
    //materialPBR.aoMap = 0;

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.normalMap"), 0);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.albedoMap"), 1);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.metallicMap"), 2);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.roughnessMap"), 3);
        glUniform1i(glGetUniformLocation(Program, "uMaterial.aoMap"), 4);
    }

}

demo_pbr::~demo_pbr()
{
    glDeleteTextures(1, &materialPBR.normalMap);
    glDeleteTextures(1, &materialPBR.albedoMap);
    glDeleteTextures(1, &materialPBR.metallicMap);
    glDeleteTextures(1, &materialPBR.roughnessMap);
    glDeleteTextures(1, &materialPBR.aoMap);

    glDeleteBuffers(1, &sphere.MeshBuffer);
    glDeleteBuffers(1, &LightsUniformBuffer);

    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

void demo_pbr::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

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

    glUniform1i(glGetUniformLocation(Program, "uMaterial.isTextured"), materialPBR.isTextured);
    glUniform1i(glGetUniformLocation(Program, "uMaterial.hasNormalMap"), materialPBR.hasNormal);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uMaterial.color"), 1, GL_FALSE, materialPBR.color.e);
    glUniform3fv(glGetUniformLocation(Program, "uMaterial.albedo"), 1, materialPBR.albedo.e);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.metallic"), materialPBR.metallic);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.roughness"), materialPBR.roughness);
    glUniform1f(glGetUniformLocation(Program, "uMaterial.ao"), materialPBR.ao);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, LightsUniformBuffer);

    glActiveTexture(GL_TEXTURE0);
    if (materialPBR.normalMap != 0)
        glBindTexture(GL_TEXTURE_2D, materialPBR.normalMap);

    glActiveTexture(GL_TEXTURE1);
    if (materialPBR.albedoMap != 0)
        glBindTexture(GL_TEXTURE_2D, materialPBR.albedoMap);

    glActiveTexture(GL_TEXTURE2);
    if (materialPBR.metallicMap != 0)
        glBindTexture(GL_TEXTURE_2D, materialPBR.metallicMap);

    glActiveTexture(GL_TEXTURE3);
    if (materialPBR.roughnessMap != 0)
        glBindTexture(GL_TEXTURE_2D, materialPBR.roughnessMap);

    glActiveTexture(GL_TEXTURE4);
    if (materialPBR.aoMap != 0)
        glBindTexture(GL_TEXTURE_2D, materialPBR.aoMap);

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
            ImGui::Checkbox("IsTextured", &materialPBR.isTextured);

            if (!enableSceneMultiSphere)
            {


                if (ImGui::TreeNodeEx("Material"))
                {
                    ImGui::Checkbox("hasNormal", &materialPBR.hasNormal);
                    ImGui::ColorEdit4("Color", materialPBR.color.e);
                    ImGui::ColorEdit3("Albedo", materialPBR.albedo.e);
                    ImGui::SliderFloat("Metallic", &materialPBR.metallic, 0.f, 1.f);
                    ImGui::SliderFloat("Roughness", &materialPBR.roughness, 0.f, 1.f);

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

