
#include <imgui.h>

#include "platform.h"

#include "color.h"

#include "tavern_scene.h"

tavern_scene::tavern_scene(GL::cache& GLCache)
{
    // Init lights
    {
        this->LightCount = 6;
        this->Lights.resize(this->LightCount);

        // (Default light, standard values)
        GL::light DefaultLight = {};
        DefaultLight.Enabled     = true;
        DefaultLight.Position    = { 0.0f, 0.0f, 0.0f, 1.f };
        DefaultLight.Ambient     = { 0.2f, 0.2f, 0.2f };
        DefaultLight.Diffuse     = { 1.0f, 1.0f, 1.0f };
        DefaultLight.Specular    = { 0.0f, 0.0f, 0.0f };
        DefaultLight.Attenuation = { 1.0f, 0.0f, 0.0f };

        // Sun light
        this->Lights[0] = DefaultLight;
        this->Lights[0].Position = { 1.f, 3.f, 1.f, 0.f }; // Directional light
        this->Lights[0].Diffuse = Color::RGB(0x374D58);

        // Candles
        GL::light CandleLight = DefaultLight;
        CandleLight.Diffuse = Color::RGB(0xFFB400);
        CandleLight.Specular = CandleLight.Diffuse;
        CandleLight.Attenuation = { 0.f, 0.f, 2.0f };

        this->Lights[1] = this->Lights[2] = this->Lights[3] = this->Lights[4] = this->Lights[5] = CandleLight;

        // Candle positions (taken from mesh data)
        this->Lights[1].Position = { -3.214370f,-0.162299f, 5.547660f, 1.f }; // Candle 1
        this->Lights[2].Position = { -4.721620f,-0.162299f, 2.590890f, 1.f }; // Candle 2
        this->Lights[3].Position = { -2.661010f,-0.162299f, 0.235029f, 1.f }; // Candle 3
        this->Lights[4].Position = {  0.012123f, 0.352532f,-2.302700f, 1.f }; // Candle 4
        this->Lights[5].Position = {  3.030360f, 0.352532f,-1.644170f, 1.f }; // Candle 5

    }

    // Create mesh
    {
        // Use vbo from GLCache
        MeshBuffer = GLCache.LoadObj("media/fantasy_game_inn.obj", 1.f, &this->MeshVertexCount);
        
        MeshDesc.Stride = sizeof(vertex_full);
        MeshDesc.HasNormal = true;
        MeshDesc.HasUV = true;
        MeshDesc.PositionOffset = OFFSETOF(vertex_full, Position);
        MeshDesc.UVOffset = OFFSETOF(vertex_full, UV);
        MeshDesc.NormalOffset = OFFSETOF(vertex_full, Normal);
    }

    // Gen texture
    {
        DiffuseTexture  = GLCache.LoadTexture("media/fantasy_game_inn_diffuse.png", IMG_FLIP | IMG_GEN_MIPMAPS);
        EmissiveTexture = GLCache.LoadTexture("media/fantasy_game_inn_emissive.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    }
    
    // Gen light uniform buffer
    {
        glGenBuffers(1, &LightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, LightCount * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
    }
}

tavern_scene::~tavern_scene()
{
    glDeleteBuffers(1, &LightsUniformBuffer);
    //glDeleteTextures(1, &Texture);   // From cache
    //glDeleteBuffers(1, &MeshBuffer); // From cache
}

static bool EditLight(GL::light* Light)
{
    bool Result =
          ImGui::Checkbox("Enabled", (bool*)&Light->Enabled)
        + ImGui::SliderFloat4("Position", Light->Position.e, -4.f, 4.f)
        + ImGui::ColorEdit3("Ambient", Light->Ambient.e)
        + ImGui::ColorEdit3("Diffuse", Light->Diffuse.e)
        + ImGui::ColorEdit3("Specular", Light->Specular.e)
        + ImGui::SliderFloat("Attenuation (constant)",  &Light->Attenuation.e[0], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (linear)",    &Light->Attenuation.e[1], 0.f, 10.f)
        + ImGui::SliderFloat("Attenuation (quadratic)", &Light->Attenuation.e[2], 0.f, 10.f);
    return Result;
}

void tavern_scene::InspectLights()
{
    if (ImGui::TreeNodeEx("Lights"))
    {
        for (int i = 0; i < LightCount; ++i)
        {
            if (ImGui::TreeNode(&Lights[i], "Light[%d]", i))
            {
                GL::light& Light = Lights[i];
                if (EditLight(&Light))
                {
                    glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(GL::light), sizeof(GL::light), &Light);
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
}
