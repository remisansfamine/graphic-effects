
#include <vector>

#include <imgui.h>
#include <filesystem>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_skybox.h"

#include "pg.h"

#include "stb_image.h"

// Vertex format
// ==================================================
struct vertex
{
    v3 Position;
    v2 UV;
};

const int LIGHT_BLOCK_BINDING_POINT = 0;

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

void main()
{
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz / pos4.w;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
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
// Varyings
in vec2 vUV;

// Uniforms
uniform sampler2D uColorTexture;

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

vec3 grayScale(in vec3 color, in vec3 colorSpectrum = vec3(1.0, 1.0, 1.0))
{
    float average = dot(colorSpectrum, color) / 3.0;
    return vec3(average);
}

vec4 grayScale(in vec4 color, in vec3 colorSpectrum = vec3(1.0, 1.0, 1.0))
{
    return vec4(grayScale(color.rgb, colorSpectrum), 1.0);
}

const float offset = 1.0 / 300.0;  

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
    vec4 pureColor = texture(uColorTexture, vUV);

    oColor = kernelEffect(edgeDetection);

    //oColor = grayScale(pureColor, vec3(0.2126 , 0.7152 , 0.0722));
    //oColor = invert(pureColor);
})GLSL";

const std::string skyboxFaces[6] = {
    "D:\Projects\GP2\Engine\PostProcess\ibr\media\Sky_NightTime01BK.png",
    "D:\Projects\GP2\Engine\PostProcess\ibr\media\Sky_NightTime01DN.png",
    "D:\Projects\GP2\Engine\PostProcess\ibr\media\Sky_NightTime01FT.png",
    "D:\Projects\GP2\Engine\PostProcess\ibr\media\Sky_NightTime01LF.png",
    "D:\Projects\GP2\Engine\PostProcess\ibr\media\Sky_NightTime01RT.png",
    "D:\Projects\GP2\Engine\PostProcess\ibr\media\Sky_NightTime01UP.png"
};

#include <iostream>
demo_skybox::demo_skybox(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), DemoBase(GLCache, GLDebug)
{
    // Generate ID texture
    glGenTextures(1, &Skybox.ID);
    glBindTexture(GL_TEXTURE_2D, Skybox.ID);

    stbi_set_flip_vertically_on_load(false);

    int channel;
    unsigned int dimIndex, dimIndexUp;

    // Load and generate skybox faces
    for (unsigned int i = 0; i < 6; i++)
    {
        dimIndex = i * 2;
        dimIndexUp = dimIndex + 1;

        int dimX, dimY;

        unsigned char* data = stbi_load(skyboxFaces[i].c_str(), &dimX, &dimY, &channel, 0);

        if (!data)
        {
            // Error on load (missing textures or fail open file)
            const char* error = stbi_failure_reason();
            std::string errorStr = error + ' ';

            std::cout << error + skyboxFaces[i] << std::endl;
            continue;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, dimX, dimY, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    // Set textures parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Gen cube and its program
    {
        Program = GL::CreateProgram(gPPVertShaderStr, gPPFragShaderStr);

        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex, Position);
        Descriptor.UVOffset = OFFSETOF(vertex, UV);

        // Create cube vertices
        vertex Cube[36];
        Skybox.VertexCount = 36;
        Mesh::BuildCube(Cube, Cube + Skybox.VertexCount, Descriptor);

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &Skybox.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, Skybox.VBO);
        glBufferData(GL_ARRAY_BUFFER, Skybox.VertexCount * sizeof(vertex), Cube, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &Skybox.VAO);
        glBindVertexArray(Skybox.VAO);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(Descriptor.PositionOffset));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(Descriptor.UVOffset));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

  demo_skybox::~demo_skybox()
{
    // Cleanup GL
    glDeleteBuffers(1, &Skybox.VBO);
    glDeleteVertexArrays(1, &Skybox.VAO);
    glDeleteProgram(Program);
}

void demo_skybox::Update(const platform_io& IO)
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthMask(0);

    glUseProgram(Program);
    glBindVertexArray(VAO);

    // Set uniform

    // Bind texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox.ID);

    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);

    // TODO : Cast to mat3
    mat4 CastView = mat4(CameraGetInverseMatrix(Camera));
    mat4 VPMatrix = ProjectionMatrix * CastView;

    // Draw
    glUniformMatrix4fv(glGetUniformLocation(Program, "uViewProj"), 1, GL_FALSE, VPMatrix.e);
    glDrawArrays(GL_TRIANGLES, 0, Skybox.VertexCount);

    glDepthMask(1);

    DemoBase.Update(IO);
}