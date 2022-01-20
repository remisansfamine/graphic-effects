
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_fbo.h"

#include "pg.h"

// Vertex format
// ==================================================
struct vertex
{
    v3 Position;
    v2 UV;
};

const int LIGHT_BLOCK_BINDING_POINT = 0;

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
    vPos = pos4.xyz / pos4.w;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
#version 330 core

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(vNormal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse  += light.diffuse;
        lightResult.specular += light.specular;
    }
    return lightResult;
}

void main()
{
    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";

// Shaders
// ==================================================
static const char* gPPVertShaderStr = R"GLSL(
#version 330 core

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;

// Uniforms
uniform mat4 uModelViewProj;

// Varyings (variables that are passed to fragment shader with perspective interpolation)
out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = uModelViewProj * vec4(aPosition, 1.0);
})GLSL";

static const char* gPPFragShaderStr = R"GLSL(
#version 330 core

// Varyings
in vec2 vUV;

// Uniforms
uniform sampler2D uColorTexture;
uniform int transformType;
uniform float offset;

// Shader outputs
out vec4 oColor;

vec3 invert(in vec3 color)
{
    return 1.0 - color;
}

vec4 invert(in vec4 color)
{
    return vec4(invert(color.rgb), 1.0);
}

vec3 sepia(in vec3 color)
{
    vec3 result;
    result.r = dot(color, vec3(0.393, 0.769, 0.189));
    result.g = dot(color, vec3(0.349, 0.686, 0.168));
    result.b = dot(color, vec3(0.272, 0.534, 0.131));

    return result;
}

vec4 sepia(in vec4 color)
{
    return vec4(sepia(color.rgb), 1.0);
}

vec3 grayScale(in vec3 color, in vec3 colorSpectrum = vec3(1.0, 1.0, 1.0))
{
    float average = dot(colorSpectrum, color) / 3.0;
    return vec3(average);
}

vec4 grayScale(in vec4 color, in vec3 colorSpectrum = vec3(1.0, 1.0, 1.0))
{
    return vec4(grayScale(color.rgb, colorSpectrum), 1.0);
}

vec2 directions[9] = vec2[](
    vec2(-1.0, 1.0), // top-left
    vec2( 0.0, 1.0), // top-center
    vec2( 1.0, 1.0), // top-right
    vec2(-1.0, 0.0),   // center-left
    vec2( 0.0, 0.0),   // center-center
    vec2( 1.0, 0.0),   // center-right
    vec2(-1.0,-1.0), // bottom-left
    vec2( 0.0,-1.0), // bottom-center
    vec2( 1.0,-1.0)  // bottom-right    
);

mat3 blurKernel = mat3(1.0, 2.0, 1.0,
                       2.0, 4.0, 2.0,
                       1.0, 2.0, 1.0) / 16.0;

mat3 edgeDetection = mat3(1.0, 1.0, 1.0,
                          1.0,-8.0, 1.0,
                          1.0, 1.0, 1.0);

vec4 kernelEffect(in mat3 kernel)
{
    vec3 result = vec3(0.0);
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            vec3 sampleTex = texture(uColorTexture, vUV + directions[i] * offset).rgb;
            result += sampleTex * kernel[i][j];
        }
    }

    return vec4(result, 1.0);
}

void main()
{
    switch (transformType)
    {
    case 0:
        oColor = texture(uColorTexture, vUV);
        break;

    case 1:
        oColor = grayScale(texture(uColorTexture, vUV), vec3(0.2126 , 0.7152 , 0.0722));
        break;

    case 2:
        oColor = invert(texture(uColorTexture, vUV));
        break;

    case 3:
        oColor = kernelEffect(edgeDetection);
        break;

    case 4:
        oColor = sepia(texture(uColorTexture, vUV));
        break;
    }
})GLSL";

demo_fbo::demo_fbo(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), DemoBase(GLCache, GLDebug)
{
    // Geneate the framebuffer and bind it
    glGenFramebuffers(1, &Framebuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.FBO);
    
    // Generate the color attachement texture
    {
        glGenTextures(1, &Framebuffer.ColorTexture);
        glBindTexture(GL_TEXTURE_2D, Framebuffer.ColorTexture);

        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, IO.WindowWidth, IO.WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Framebuffer.ColorTexture, 0);
        }
    }

    // Geneate the renderbuffer and bind it to the framebuffer
    {
        glGenRenderbuffers(1, &Framebuffer.DepthStencilRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, Framebuffer.DepthStencilRenderbuffer);

        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, IO.WindowWidth, IO.WindowHeight);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Framebuffer.DepthStencilRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Gen quad and its program
    {
        PostProcessPassData.Program = GL::CreateProgram(gPPVertShaderStr, gPPFragShaderStr);

        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex, Position);
        Descriptor.UVOffset = OFFSETOF(vertex, UV);

        // Create a cube in RAM
        vertex Quad[6];
        PostProcessPassData.VertexCount = 6;
        Mesh::BuildScreenQuad(Quad, Quad + PostProcessPassData.VertexCount, Descriptor, { 2.f, 2.f });

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &PostProcessPassData.VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, PostProcessPassData.VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, PostProcessPassData.VertexCount * sizeof(vertex), Quad, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &PostProcessPassData.VAO);
        glBindVertexArray(PostProcessPassData.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, PostProcessPassData.VertexBuffer);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, UV));

        glBindVertexArray(0);
    }
}

demo_fbo::~demo_fbo()
{
    // Delete First pass buffers
    {
        glDeleteFramebuffers(1, &Framebuffer.FBO);
        glDeleteTextures(1, &Framebuffer.ColorTexture);
        glDeleteRenderbuffers(1, &Framebuffer.DepthStencilRenderbuffer);
    }

    // Delete second pass buffers
    {
        // Cleanup GL
        glDeleteBuffers(1, &PostProcessPassData.VertexBuffer);
        glDeleteVertexArrays(1, &PostProcessPassData.VAO);
        glDeleteProgram(PostProcessPassData.Program);
    }
}

static void DrawQuad(GLuint Program, mat4 ModelViewProj)
{
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelViewProj"), 1, GL_FALSE, ModelViewProj.e);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void demo_fbo::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_fbo", ImGuiTreeNodeFlags_Framed))
    {
        const char* items[] = { "IDENTITY", "GRAYSCALE", "INVERT", "EDGEDETECTION", "SEPIA" };

        if (ImGui::BeginCombo("Color transform", items[(int)transformType], ImGuiComboFlags_NoArrowButton))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                bool is_selected = (int)transformType == n;
                if (ImGui::Selectable(items[n], is_selected))
                    transformType = color_transform(n);

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
}

void demo_fbo::Update(const platform_io& IO)
{
    if (IO.WindowSizeChanged)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.FBO);

        glBindRenderbuffer(GL_RENDERBUFFER, Framebuffer.DepthStencilRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, IO.WindowWidth, IO.WindowHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, Framebuffer.ColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, IO.WindowWidth, IO.WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // First rendering pass
    glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.FBO);
    glEnable(GL_DEPTH_TEST);

    DemoBase.Update(IO);

    // Second rendering pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(PostProcessPassData.Program);
    glBindVertexArray(PostProcessPassData.VAO);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, Framebuffer.ColorTexture);

    glUniform1i(glGetUniformLocation(PostProcessPassData.Program, "transformType"), (GLint)transformType);
    glUniform1f(glGetUniformLocation(PostProcessPassData.Program, "offset"), 1.f / 300.f);

    DrawQuad(PostProcessPassData.Program, Mat4::Identity());

    glBindVertexArray(0);
    glUseProgram(0);

    DisplayDebugUI();
}