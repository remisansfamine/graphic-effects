#include "demo_all.h"

#include <string>
#include <imgui.h>
#include <iostream>
#include <filesystem>

#include "color.h"
#include "stb_image.h"

//#define PROJECT_DIR (std::filesystem::current_path().string() + "\\")

const int LIGHT_BLOCK_BINDING_POINT = 0;

demo_all::demo_all(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug)
{
    UberProgram.ID = GL::CreateProgramFromFiles("src/uber_shader.vert", "src/uber_shader.frag");

    // Gen meshes
    {
        // Gen quad
        {
            // Create a cube in RAM
            vertex_full Quad[6];
            QuadMesh.VertexCount = 6;
            Mesh::BuildQuad(Quad, Quad + QuadMesh.VertexCount, QuadMesh.Descriptor);

            // Upload cube to gpu (VRAM)
            glGenBuffers(1, &QuadMesh.VBO.ID);
            QuadMesh.CreateBufferData(Quad);
            QuadMesh.CreateVertexArray();
        }

        // Gen Backpack
        {
            BackpackMesh.VBO.ID = GLCache.LoadObj("media/backpack.obj", 1.f, &BackpackMesh.VertexCount);
            BackpackMesh.CreateVertexArray();
        }
    }

    // Gen textures
    {
        diffuseTex.ID = GLCache.LoadTexture("media/diffuse.jpg", IMG_GEN_MIPMAPS);
        normalMap.ID = GLCache.LoadTexture("media/normal.png", IMG_GEN_MIPMAPS);
    }

    // Preload texture uniform
    {
        // Preload them
        UberProgram.bind();
        glUniform1i(glGetUniformLocation(UberProgram.ID, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(UberProgram.ID, "uNormalMap"), 1);
    }

    SetupLight();
    // Normal map (vertex shader position TBN)
    // Hdr
    // Skybox
    // Shadow map
    // Instancing
    // NPR
    // PBR
    // Picking
    // Deferred Shading
}

void demo_all::SetupLight()
{
    // Set uniforms that won't change
    {
        UberProgram.bind();
        glUniformBlockBinding(UberProgram.ID, glGetUniformBlockIndex(UberProgram.ID, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
        UberProgram.unbind();
    }

    this->Lights.resize(6);

    // (Default light, standard values)
    GL::light DefaultLight = {};
    DefaultLight.Enabled = true;
    DefaultLight.Position = { 0.0f, 0.0f, 0.0f, 1.f };
    DefaultLight.Ambient = { 0.2f, 0.2f, 0.2f };
    DefaultLight.Diffuse = { 1.0f, 1.0f, 1.0f };
    DefaultLight.Specular = { 0.0f, 0.0f, 0.0f };
    DefaultLight.Attenuation = { 1.0f, 0.0f, 0.0f };

    // Sun light
    this->Lights[0] = DefaultLight;
    this->Lights[0].Position = { 1.f, 3.f, 1.f, 0.f }; // Directional light
    this->Lights[0].Diffuse = Color::RGB(0xFFFFFF);

    // Candles
    GL::light CandleLight = DefaultLight;
    CandleLight.Diffuse = Color::RGB(0xEEEE00);
    CandleLight.Specular = CandleLight.Diffuse;
    CandleLight.Attenuation = { 0.f, 0.f, 2.0f };

    this->Lights[1] = this->Lights[2] = this->Lights[3] = this->Lights[4] = this->Lights[5] = CandleLight;

    // Candle positions (taken from mesh data)
    this->Lights[1].Position = { -3.214370f,-0.162299f, 5.547660f, 1.f }; // Candle 1
    this->Lights[2].Position = { -4.721620f,-0.162299f, 2.590890f, 1.f }; // Candle 2
    this->Lights[3].Position = { -2.661010f,-0.162299f, 0.235029f, 1.f }; // Candle 3
    this->Lights[4].Position = { 0.012123f, 0.352532f,-2.302700f, 1.f }; // Candle 4
    this->Lights[5].Position = { 3.030360f, 0.352532f,-1.644170f, 1.f }; // Candle 5

    // Gen light uniform buffer
    {
        glGenBuffers(1, &LightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, Lights.size() * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void demo_all::SetupOpenGL()
{

}


demo_all::~demo_all()
{
}

void demo_all::Update(const platform_io& IO)
{
    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Compute model-view-proj and send it to shader
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 1000.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, -5.f });

    // Setup GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Clear screen
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader and send data
    UberProgram.bind();

    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(UberProgram.ID, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(UberProgram.ID, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(UberProgram.ID, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(UberProgram.ID, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(UberProgram.ID, "uViewPosition"), 1, Camera.Position.e);

    // Bind uniform buffer and 
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, LightsUniformBuffer);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    diffuseTex.bind();

    glActiveTexture(GL_TEXTURE1);
    normalMap.bind();

    BackpackMesh.Draw();

    DisplayDebugUI();
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

void demo_all::DisplayDebugUI()
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
            for (int i = 0; i < Lights.size(); ++i)
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

        ImGui::TreePop();
    }
}

