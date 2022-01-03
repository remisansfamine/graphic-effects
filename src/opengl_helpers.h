#pragma once

#include "opengl_headers.h"
#include "types.h"
#include "opengl_helpers_cache.h"
#include "opengl_helpers_wireframe.h"

enum image_flags
{
    IMG_FLIP             = 1 << 0,
    IMG_FORCE_GREY       = 1 << 1,
    IMG_FORCE_GREY_ALPHA = 1 << 2,
    IMG_FORCE_RGB        = 1 << 3,
    IMG_FORCE_RGBA       = 1 << 4,
    IMG_GEN_MIPMAPS      = 1 << 5,
};

namespace GL
{
    // Same memory layout than 'struct light' in glsl shader
    struct light
    {
        alignas(16) int Enabled;
        alignas(16) v4 Position; // (world position)
        alignas(16) v3 Ambient;
        alignas(16) v3 Diffuse;
        alignas(16) v3 Specular;
        alignas(16) v3 Attenuation;
    };

    // Same memory layout than 'struct material' in glsl shader
    struct alignas(16) material
    {
        v4 Ambient;
        v4 Diffuse;
        v4 Specular;
        v4 Emission;
        float Shininess;
    };

    class debug
    {
    public:
        void WireframePrepare(GLuint MeshVBO, GLsizei PositionStride, GLsizei PositionOffset, int VertexCount)
        {
            Wireframe.BindBuffer(MeshVBO, PositionStride, PositionOffset, VertexCount);
        }

        void WireframeDrawArray(GLint First, GLsizei Count, const mat4& MVP)
        {
            Wireframe.DrawArray(First, Count, MVP);
        }
        
        GL::wireframe_renderer Wireframe;
    };

    void UniformLight(GLuint Program, const char* LightUniformName, const light& Light);
    void UniformMaterial(GLuint Program, const char* MaterialUniformName, const material& Material);
    GLuint CompileShader(GLenum ShaderType, const char* ShaderStr, bool InjectLightShading = false);
    GLuint CompileShaderEx(GLenum ShaderType, int ShaderStrsCount, const char** ShaderStrs, bool InjectLightShading = false);
    GLuint CreateProgram(const char* VSString, const char* FSString, bool InjectLightShading = false);
    GLuint CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringCount, const char** FSString, bool InjectLightShading = false);
    const char* GetShaderStructsDefinitions();
    void UploadTexture(const char* Filename, int ImageFlags = 0, int* WidthOut = nullptr, int* HeightOut = nullptr);
    void UploadCheckerboardTexture(int Width, int Height, int SquareSize);
}
