
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_shadowMap.h"

#include "pg.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

// Vertex format
// ==================================================
struct vertex
{
    v3 Position;
    v2 UV;
};

static const std::string skyboxFaces[6] = {
    "media\\right.jpg",
    "media\\left.jpg",
    "media\\top.jpg",
    "media\\bottom.jpg",
    "media\\front.jpg",
    "media\\back.jpg"
};

static const char* gVertexShaderStr = R"GLSL(
#version 330 core

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space

void main()
{
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
#define PCF 3
#define PCFSampleCount (1 + 2 * PCF) * (1 + 2 * PCF)
#define PCFFactor 1.0 / (PCFSampleCount)

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;
uniform sampler2D uShadowMap;

uniform mat4 uLightSpaceMatrix;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

float getDirectionalShadow(int index)
{
    // Perspcetive divide
    vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(vPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Avoid shadow out of the frustum
    if (projCoords.z > 1.0)
        return 0.0;

    // [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    vec3 lightDir = normalize(uLight[index].position.xyz - vPos);

    float slopeFactor = 1.0 - dot(vNormal, lightDir);

    float minBias = 0.00005;
    float maxBias = 0.0005;
    float bias = max(minBias * slopeFactor, maxBias);
    float currentDepth = projCoords.z - bias;

    // Apply Percentage-Closer filtering to avoid "stair" shadows
    // Use to soft shadow boders
    float shadow = 0.0;
        
    // Calculate the texel size from the depth texture size
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
        
    for (int x = -PCF; x <= PCF; x++)
    {
        for (int y = -PCF; y <= PCF; y++)
        {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;

            // Compare pcf and current depth of fragment to determine shadow
            shadow += float(currentDepth > pcfDepth);
        }
    }

    return shadow * PCFFactor;
}

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        float shadow = 1.0;
        if (i == 0)
            shadow -= getDirectionalShadow(i);

        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(vNormal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse  += light.diffuse * shadow;
        lightResult.specular += light.specular * shadow;
    }

    return lightResult;
}

void main()
{
    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient * texture(uDiffuseTexture, vUV).rgb;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";

demo_shadowMap::demo_shadowMap(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), TavernScene(GLCache)//DemoBase(GLCache, GLDebug)
{
    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
    }

    // Create a vertex array and bind attribs onto the vertex buffer
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, TavernScene.MeshBuffer);

        vertex_descriptor& Desc = TavernScene.MeshDesc;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.NormalOffset);
    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);
        glUniform1i(glGetUniformLocation(Program, "uShadowMap"), 2);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
        glUseProgram(0);
    }




    Shadow.width = (unsigned int)((float)Shadow.width * Shadow.superSample);
    Shadow.height = (unsigned int)((float)Shadow.height * Shadow.superSample);

    Shadow.aspect = (float)Shadow.width / (float)Shadow.height;

    Shadow.Program = GL::CreateProgramFromFiles("src/shaders/shadow_shader.vert", "src/shaders/shadow_shader.frag");

    // Generate shadow map
    glGenFramebuffers(1, &Shadow.FBO);

    // Generate shadow texture
    glGenTextures(1, &Shadow.ID);
    glBindTexture(GL_TEXTURE_2D, Shadow.ID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Shadow.width, Shadow.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Clamp to border for deactivate frustum shadows
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.f, 1.f, 1.f, 1.f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, Shadow.FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Shadow.ID, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* //Generate VAO
    glGenVertexArrays(1, &Shadow.VAO);
    glBindVertexArray(Shadow.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, TavernScene.MeshBuffer);

    vertex_descriptor& Desc = TavernScene.MeshDesc;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);*/

    // Create program
}

demo_shadowMap::~demo_shadowMap()
{
    // Cleanup GL
    if (Shadow.ID)
        glDeleteTextures(1, &Shadow.ID);
    if (Shadow.FBO)

    if (VertexBuffer)
        glDeleteBuffers(1, &VertexBuffer);
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (Program)
        glDeleteProgram(Program);
}

void demo_shadowMap::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_shadowMap", ImGuiTreeNodeFlags_Framed))
    {
        TavernScene.InspectLights();

        ImGui::Image((ImTextureID)Shadow.ID, ImVec2(200.f, 200.f));
        ImGui::TreePop();
    }
}

void demo_shadowMap::CreateShadowTexture(const platform_io& IO)
{
    glCullFace(GL_FRONT);

    glUseProgram(Shadow.Program);

    mat4 lightView = Mat4::LookAt(TavernScene.Lights[0].Position.xyz);
    mat4 ortho = Mat4::Transpose(CameraGetOrthographicShadow(Camera));
    mat4 lightSpaceMatrix = ortho  * lightView;
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    // Send uniforms
    glUniformMatrix4fv(glGetUniformLocation(Shadow.Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Shadow.Program, "uLightSpaceMatrix"), 1, GL_FALSE, lightSpaceMatrix.e);

    // Set shadow texture viewport
    glViewport(0, 0, Shadow.width, Shadow.height);
    glBindFramebuffer(GL_FRAMEBUFFER, Shadow.FBO);

    glClear(GL_DEPTH_BUFFER_BIT);

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);

    // Reset viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glCullFace(GL_BACK);
}

void demo_shadowMap::RenderTavern(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    mat4 lightView = Mat4::LookAt(TavernScene.Lights[0].Position.xyz);
    mat4 ortho = Mat4::Transpose(CameraGetOrthographicShadow(Camera));
    mat4 lightSpaceMatrix = ortho * lightView;

    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uLightSpaceMatrix"), 1, GL_FALSE, lightSpaceMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TavernScene.DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TavernScene.EmissiveTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, Shadow.ID);
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);
}

void demo_shadowMap::Update(const platform_io& IO)
{
    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    CreateShadowTexture(IO);
    RenderTavern(IO);

    glDisable(GL_DEPTH_TEST);

    DisplayDebugUI();
}