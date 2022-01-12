
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_npr.h"

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
static const char* gVertexShaderOutStr = R"GLSL(
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
out vec3 vViewDir;

void main()
{
    vViewDir = vec3(uView[0][3], uView[1][3], uView[2][3]);
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";

static const char* gFragmentShaderOutStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in vec3 vViewDir;

// Uniforms
uniform vec3 uColor;
uniform vec4 uLightPos;

uniform bool uUsePalette;

uniform sampler2D uToonPalette;

// Shader outputs
out vec4 oColor;

vec3 ToonShadingPalette(vec3 LightPos, vec3 Normal)
{
    float NL = dot(normalize(LightPos),Normal);

    float intensity = clamp(NL, 0.0, 1.0);
    
    vec2 UVs = vec2(intensity, 0.0);

    return texture(uToonPalette, UVs).rgb;
}

vec3 ToonShading(vec3 LightPos, vec3 Normal)
{
    float intensity = dot(normalize(LightPos),Normal);

    if (intensity > 0.95)      return vec3(1.0);
    else if (intensity > 0.75) return vec3(0.8);
    else if (intensity > 0.50) return vec3(0.6);
    else if (intensity > 0.25) return vec3(0.4);
    else                       return vec3(0.2);
}

void main()
{
    vec3 colorAttenuation;

    if (uUsePalette)
        colorAttenuation = ToonShadingPalette(uLightPos.xyz, vNormal);
    else
        colorAttenuation = ToonShading(uLightPos.xyz, vNormal);

    oColor = vec4(uColor * colorAttenuation, 1.0);
})GLSL";

// Shaders
// ==================================================
static const char* gVertexShaderStr = R"GLSL(
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
out vec3 vViewDir;

void main()
{
    vViewDir = vec3(uView[0][3], uView[1][3], uView[2][3]);
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in vec3 vViewDir;

// Uniforms
uniform vec3 uColor;
uniform vec4 uLightPos;

uniform bool uUsePalette;

uniform sampler2D uToonPalette;

// Shader outputs
out vec4 oColor;

vec3 ToonShadingPalette(vec3 LightPos, vec3 Normal)
{
    float NL = dot(normalize(LightPos),Normal);

    float intensity = clamp(NL, 0.0, 1.0);
    
    vec2 UVs = vec2(intensity, 0.0);

    return texture(uToonPalette, UVs).rgb;
}

vec3 ToonShading(vec3 LightPos, vec3 Normal)
{
    float intensity = dot(normalize(LightPos),Normal);

    if (intensity > 0.95)      return vec3(1.0);
    else if (intensity > 0.75) return vec3(0.8);
    else if (intensity > 0.50) return vec3(0.6);
    else if (intensity > 0.25) return vec3(0.4);
    else                       return vec3(0.2);
}

void main()
{
    vec3 colorAttenuation;

    if (uUsePalette)
        colorAttenuation = ToonShadingPalette(uLightPos.xyz, vNormal);
    else
        colorAttenuation = ToonShading(uLightPos.xyz, vNormal);

    oColor = vec4(uColor * colorAttenuation, 1.0);
})GLSL";

demo_npr::demo_npr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : GLCache(GLCache), GLDebug(GLDebug)
{
    // Create render pipeline
    Program = GL::CreateProgram(gVertexShaderStr, gFragmentShaderStr);

    // Gen mesh
    {
        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.HasNormal = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.NormalOffset = OFFSETOF(vertex_full, Normal);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);

        VertexBuffer = GLCache.LoadObj("media/T-Rex.obj", 1.f, &VertexCount);

        // Create a vertex array
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        {
            //glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.PositionOffset));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.UVOffset));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.NormalOffset));

            glEnableVertexAttribArray(0);
        }
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Gen and bind palette texture
    {

        Texture = GLCache.LoadTexture("media/ToonPalette.png", IMG_GEN_MIPMAPS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glUseProgram(Program);

        glUniform1i(glGetUniformLocation(Program, "uToonPalette"), 0);
        glUniform1i(glGetUniformLocation(Program, "uUsePalette"), (int)usePalette);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    // Generate FBO for outline
    {
        // Geneate the framebuffer and bind it
        glGenFramebuffers(1, &OutlineFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, OutlineFBO);

        // Generate the color attachement texture
        {
            glGenTextures(1, &OutlineTexture);
            glBindTexture(GL_TEXTURE_2D, OutlineTexture);

            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, IO.WindowWidth, IO.WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, OutlineTexture, 0);
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        OutlineProgram = GL::CreateProgram(gVertexShaderOutStr, gFragmentShaderOutStr);

        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex, Position);
        Descriptor.UVOffset = OFFSETOF(vertex, UV);

        // Create a cube in RAM
        vertex Quad[6];
        Mesh::BuildScreenQuad(Quad, Quad + 6, Descriptor, { 2.f, 2.f });

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &QuadFBO);
        glBindBuffer(GL_ARRAY_BUFFER, QuadFBO);
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vertex), Quad, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &QuadVAO);
        glBindVertexArray(QuadVAO);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, UV));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

demo_npr::~demo_npr()
{
    // Cleanup GL
    glDeleteBuffers(1, &VertexBuffer);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

void demo_npr::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_npr", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::DragFloat4("LightPos", lightPos.e, 0.1f);
        ImGui::ColorEdit3("Color", color.e);

        if (ImGui::Checkbox("Use palette texture", &usePalette))
        {
            glUseProgram(Program);
            glUniform1i(glGetUniformLocation(Program, "uUsePalette"), (int)usePalette);
            glUseProgram(0);
        }

        ImGui::TreePop();
    }
}

void demo_npr::RenderOutline()
{
    glUseProgram(OutlineProgram);
}

void demo_npr::Update(const platform_io& IO)
{
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Compute model-view-proj and send it to shader
    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), (float)IO.WindowWidth / (float)IO.WindowHeight, 0.1f, 1000.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));

    // Setup GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glCullFace(GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //glDepthFunc(GL_LESS);

    // Clear screen
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(Program);

    // Use shader and send data
    glUniform4fv(glGetUniformLocation(Program, "uLightPos"), 1, lightPos.e);
    glUniform3fv(glGetUniformLocation(Program, "uColor"), 1, color.e);
    //glUniform3fv(glGetUniformLocation(Program, "uViewDir"), 1, Camera.Position.e);

    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);

    //glBindFramebuffer(GL_FRAMEBUFFER, OutlineFBO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, VertexCount);
    glBindVertexArray(0);

    glUseProgram(0);

    RenderOutline();

    DisplayDebugUI();
}
