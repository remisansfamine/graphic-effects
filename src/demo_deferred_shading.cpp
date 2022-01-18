
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_deferred_shading.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

static const char* gGeoVertexShaderStr = R"GLSL(
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

static const char* gGeoFragmentShaderStr = R"GLSL(
// Varyings
in vec3 vPos;
in vec2 vUV;
in vec3 vNormal;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

// Shader outputs
layout (location = 0) out vec3 oPosition;
layout (location = 1) out vec3 oNormal;
layout (location = 2) out vec4 oAlbedo;
layout (location = 3) out vec4 oEmissive;

void main()
{
    oPosition = vPos;

    oNormal = normalize(vNormal);

    // Apply light color
    oAlbedo = texture(uDiffuseTexture, vUV);

    // Apply light color
    oEmissive = texture(uEmissiveTexture, vUV);
})GLSL";

static const char* gLightVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;

// Varyings
out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = vec4(aPosition, 1.0);
})GLSL";

static const char* gLightFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;

uniform sampler2D uPosition;
uniform sampler2D uNormal;
uniform sampler2D uAlbedo;
uniform sampler2D uEmissive;

// Uniforms
uniform vec3 uViewPosition;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

light_shade_result get_lights_shading(in vec3 fragPos, in vec3 normal)
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light_shade_result currLight = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, fragPos, normal);
        lightResult.ambient  += currLight.ambient;
        lightResult.diffuse  += currLight.diffuse;
        lightResult.specular += currLight.specular;
    }
    return lightResult;
}

void main()
{
    vec3 fragPos = texture(uPosition, vUV).rgb;
    vec3 normal = texture(uNormal, vUV).rgb;
    vec3 albedo = texture(uAlbedo, vUV).rgb;
    vec3 emissive = texture(uEmissive, vUV).rgb;

    // Compute phong shading
    light_shade_result lightResult = get_lights_shading(fragPos, normal);
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * albedo;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient * albedo;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + emissive;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";

demo_deferred_shading::demo_deferred_shading(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), TavernScene(GLCache)
{
    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* GeoFragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gGeoFragmentShaderStr,
        };

        // Assemble fragment shader strings (defines + code)
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* LightFragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gLightFragmentShaderStr,
        };

        geometryProgram = GL::CreateProgramEx(1, &gGeoVertexShaderStr, 2, GeoFragmentShaderStrs);

        lightingProgram = GL::CreateProgramEx(1, &gLightVertexShaderStr, 2, LightFragmentShaderStrs, true);
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
        glUseProgram(geometryProgram);
        glUniform1i(glGetUniformLocation(geometryProgram, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(geometryProgram, "uEmissiveTexture"), 1);

        glUseProgram(lightingProgram);
        glUniformBlockBinding(lightingProgram, glGetUniformBlockIndex(lightingProgram, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
        glUniform1i(glGetUniformLocation(lightingProgram, "uPosition"), 0);
        glUniform1i(glGetUniformLocation(lightingProgram, "uNormal"), 1);
        glUniform1i(glGetUniformLocation(lightingProgram, "uAlbedo"), 2);
        glUniform1i(glGetUniformLocation(lightingProgram, "uEmissive"), 3);
    }

    // Generate FBO
    {
        glGenFramebuffers(1, &geometryBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, geometryBuffer);

        // Generate textures
        {
            // - position color buffer
            glGenTextures(1, &positionTexture);
            glBindTexture(GL_TEXTURE_2D, positionTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, IO.ScreenWidth, IO.ScreenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionTexture, 0);

            // - normal color buffer
            glGenTextures(1, &normalTexture);
            glBindTexture(GL_TEXTURE_2D, normalTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, IO.ScreenWidth, IO.ScreenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture, 0);

            // - color + specular color buffer
            glGenTextures(1, &albedoTexture);
            glBindTexture(GL_TEXTURE_2D, albedoTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, IO.ScreenWidth, IO.ScreenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedoTexture, 0);

            // - color + specular color buffer
            glGenTextures(1, &emissiveTexture);
            glBindTexture(GL_TEXTURE_2D, emissiveTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, IO.ScreenWidth, IO.ScreenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, emissiveTexture, 0);
        }

        // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
        GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, attachments);

        // Generate RBO
        {
            glGenRenderbuffers(1, &rboDepth);
            glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, IO.ScreenWidth, IO.ScreenHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
        }
    }

    // Generate Quad VAO
    {
        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);

        // Create a cube in RAM
        quad.VertexCount = 6;
        Mesh::BuildScreenQuad(quad.vertices, quad.vertices + quad.VertexCount, Descriptor, { 2.f, 2.f });

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &quad.VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, quad.VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, quad.VertexCount * sizeof(vertex_full), (void*)quad.vertices, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &quad.VAO);
        glBindVertexArray(quad.VAO);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)OFFSETOF(vertex_full, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)OFFSETOF(vertex_full, UV));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

demo_deferred_shading::~demo_deferred_shading()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(geometryProgram);
    glDeleteProgram(lightingProgram);

    glDeleteFramebuffers(1, &geometryBuffer);
    glDeleteRenderbuffers(1, &rboDepth);

    glDeleteTextures(1, &positionTexture);
    glDeleteTextures(1, &normalTexture);
    glDeleteTextures(1, &albedoTexture);
    glDeleteTextures(1, &emissiveTexture);
}

void demo_deferred_shading::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    glUseProgram(geometryProgram);

    glBindFramebuffer(GL_FRAMEBUFFER, geometryBuffer);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, albedoTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, emissiveTexture);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    // Render tavern
    this->RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix);
    
    glUseProgram(lightingProgram);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform3fv(glGetUniformLocation(geometryProgram, "uViewPosition"), 1, Camera.Position.e);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, albedoTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, emissiveTexture);

    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);

    glBindVertexArray(quad.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_deferred_shading::DisplayDebugUI()
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
        TavernScene.InspectLights();

        ImGui::TreePop();
    }
}

void demo_deferred_shading::RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glEnable(GL_DEPTH_TEST);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(geometryProgram, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(geometryProgram, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(geometryProgram, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(geometryProgram, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    
    // Bind uniform buffer and textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TavernScene.DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TavernScene.EmissiveTexture);
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case
    
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);
}
