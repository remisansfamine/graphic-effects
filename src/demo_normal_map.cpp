
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_normal_map.h"

#include "pg.h"

// Vertex format
// ==================================================
struct vertex
{
    v3 Position;
    v2 UV;
};

// Shaders
// ==================================================
static const char* gVertexShaderStr = R"GLSL(
#version 330 core

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space
out mat3 vTBN;

void main()
{
    vec4 pos4 = uModel * vec4(aPosition, 1.0);
    vPos = vec3(pos4);
    vUV = aUV;

    vec3 T = normalize(mat3(uModelNormalMatrix) * aTangent);
    vec3 N = normalize(mat3(uModelNormalMatrix) * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    vTBN = transpose(mat3(T, B, N));

    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;

    gl_Position = uProjection * uView * pos4;
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in mat3 vTBN;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uNormalMap;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

light_shade_result get_lights_shading(in vec3 normal)
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light currLight = uLight[i];
        currLight.position = vec4(vTBN * currLight.position.xyz, currLight.position.w);

        light_shade_result currLightResult = light_shade(currLight, gDefaultMaterial.shininess, vTBN * uViewPosition, vTBN * vPos, normal);
        lightResult.ambient  += currLightResult.ambient;
        lightResult.diffuse  += currLightResult.diffuse;
        lightResult.specular += currLightResult.specular;
    }
    return lightResult;
}

void main()
{
    vec3 normal = texture(uNormalMap, vUV).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    // Compute phong shading
    light_shade_result lightResult = get_lights_shading(normal);
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor), 1.0);
})GLSL";

demo_normal_map::demo_normal_map(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug)
{
    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", 8);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        this->Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
    }
    
    vertex_descriptor Descriptor = {};
    // Gen mesh
    {
        // Create a descriptor based on the `struct vertex` format
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.HasNormal = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);
        Descriptor.NormalOffset = OFFSETOF(vertex_full, Normal);
        Descriptor.TangentOffset = OFFSETOF(vertex_full, Tangent);
        Descriptor.BitangentOffset = OFFSETOF(vertex_full, Bitangent);

        // Create a cube in RAM
        vertex_full Quad[6];
        this->VertexCount = 6;
        Mesh::BuildQuad(Quad, Quad + this->VertexCount, Descriptor);

        //VertexBuffer = GLCache.LoadObj("media/plane.obj", 1.f, &this->VertexCount);

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &this->VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, this->VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, this->VertexCount * sizeof(vertex_full), Quad, GL_STATIC_DRAW);
    }

    // Gen texture
    {
        DiffuseTexture = GLCache.LoadTexture("media/brickwall.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);
        NormalTexture = GLCache.LoadTexture("media/brickwall_normal.jpg", IMG_FLIP | IMG_GEN_MIPMAPS);

        // Preload them
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uNormalMap"), 1);

        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }
    
    // Create a vertex array
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, this->VertexBuffer);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)Descriptor.PositionOffset);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)Descriptor.UVOffset);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)Descriptor.NormalOffset);

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)Descriptor.TangentOffset);

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)Descriptor.BitangentOffset);
    }

    // Init lights
    {
        this->LightCount = 6;
        this->Lights.resize(this->LightCount);

        // (Default light, standard values)
        GL::light DefaultLight = {};
        DefaultLight.Enabled = true;
        DefaultLight.Position = { 0.0f, 0.0f, 0.0f, 1.f };
        DefaultLight.Ambient = { 0.2f, 0.2f, 0.2f };
        DefaultLight.Diffuse = { 1.0f, 1.0f, 1.0f };
        DefaultLight.Specular = { 1.0f, 1.0f, 1.0f };
        DefaultLight.Attenuation = { 1.0f, 0.0f, 0.0f };

        // Sun light
        this->Lights[0] = DefaultLight;
        this->Lights[0].Position = { 0.f, 0.f, 0.f, 1.f }; // Directional light

        // Candles
        GL::light CandleLight = DefaultLight;
        //CandleLight.Diffuse = Color::RGB(0xFFB400);
        CandleLight.Diffuse = Color::RGB(0);
        CandleLight.Specular = CandleLight.Diffuse;
        CandleLight.Attenuation = { 0.f, 0.f, 20.0f };

        this->Lights[1] = this->Lights[2] = this->Lights[3] = this->Lights[4] = this->Lights[5] = CandleLight;

        // Candle positions (taken from mesh data)
        this->Lights[1].Position = { -3.214370f,-0.162299f, 5.547660f, 1.f }; // Candle 1
        this->Lights[2].Position = { -4.721620f,-0.162299f, 2.590890f, 1.f }; // Candle 2
        this->Lights[3].Position = { -2.661010f,-0.162299f, 0.235029f, 1.f }; // Candle 3
        this->Lights[4].Position = { 0.012123f, 0.352532f,-2.302700f, 1.f }; // Candle 4
        this->Lights[5].Position = { 3.030360f, 0.352532f,-1.644170f, 1.f }; // Candle 5

    }

    // Gen light uniform buffer
    {
        glGenBuffers(1, &LightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, LightCount * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

    }
}

demo_normal_map::~demo_normal_map()
{
    // Cleanup GL
    glDeleteTextures(1, &DiffuseTexture);
    glDeleteTextures(1, &NormalTexture);
    glDeleteBuffers(1, &VertexBuffer);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

static void DrawQuad(GLuint Program, mat4 ModelViewProj)
{
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelViewProj"), 1, GL_FALSE, ModelViewProj.e);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void demo_normal_map::Update(const platform_io& IO)
{
    InspectLights();

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Compute model-view-proj and send it to shader
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, -5.f }) * Mat4::Scale({ 10.f, 10.f, 10.f });

    // Setup GL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Clear screen
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use shader and send data
    glUseProgram(Program);
    
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);

    // Bind uniform buffer and 
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, LightsUniformBuffer);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, DiffuseTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, NormalTexture);

    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

    glBindVertexArray(VAO);

    glDrawArrays(GL_TRIANGLES, 0, VertexCount);

    // Draw origin
    PG::DebugRenderer()->DrawAxisGizmo(Mat4::Translate({ 0.f, 0.f, 0.f }), true, false);
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

void demo_normal_map::InspectLights()
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