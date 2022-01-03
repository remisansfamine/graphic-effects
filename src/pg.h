#pragma once

#include <cstdarg>

#include "opengl_headers.h"

#include "types.h"

namespace GL
{
    class wireframe_renderer;
    class cache;
}

namespace PG
{

class debug_renderer
{
public:
    virtual void SetCurrentColor(v3 Color) = 0;
    virtual void DrawTextV(v3 WorldPosition, v2 Offset, const char* Format, va_list List) = 0;
    virtual void DrawText(v3 WorldPosition, v2 Offset, const char* Format, ...) = 0;
    virtual void DrawLine(v3 Start, v3 End) = 0;
    virtual void DrawCameraGizmo(v3 Position, float Pitch, float Yaw) = 0;
    virtual void DrawCameraGizmo(const mat4& ModelMatrix) = 0;
    virtual void DrawAxisGizmo(mat4 ModelMatrix, bool ShowPos, bool ShowMatrix = false) = 0;
    virtual void Flush(const mat4& ViewProj, int ViewportX, int ViewportY, int ViewportWidth, int ViewportHeight, int WindowWidth, int WindowHeight) = 0;
};

void Init();
void Destroy();
debug_renderer* DebugRenderer();

// Basic functions
// Inverse of the (already inversed) view lookat function
mat4 ObjectLookAt(v3 Position, v3 Target, v3 Up);
// Alternate version of ObjectLookAt
mat4 ObjectLookAt2(mat4 View, v3 Position, v3 Up);
// Compute viewport matrix
mat4 ViewportMatrix(int x, int y, int Width, int Height);

// Get billboard model matrix
mat4 GetBillboardMatrix(mat4 View, v3 Position);

// Get sprite mvp matrix
mat4 GetSpriteModelViewProjMatrix(const mat4& Projection, const mat4& View, int ViewportWidth, int ViewportHeight, v3 WorldPos, v2 SpriteSize);

}