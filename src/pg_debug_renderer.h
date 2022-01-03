#pragma once

#include <vector>
#include <string>

#include "pg.h"

#include "opengl_headers.h"

#include "types.h"

class debug_renderer_impl : public PG::debug_renderer
{
    struct line_vertex
    {
        v2 Position;
        v3 Color;
    };

    enum class command_type : int
    {
        SET_CURRENT_COLOR,
        DRAW_WORLD_LINE,
        DRAW_WORLD_TEXT,
    };

    struct command
    {
        command_type Type;
        union
        {
            struct
            {
                v3 Start;
                v3 End;
            } Line;

            struct
            {
                v3 WorldPosition;
                v2 Offset;
                int StringBufferStart;
            } Text;

            v3 Color;
        };
    };

public:
    debug_renderer_impl();
    ~debug_renderer_impl();

    void SetCurrentColor(v3 Color);
    void DrawTextV(v3 WorldPosition, v2 Offset, const char* Format, va_list List);
    void DrawText(v3 WorldPosition, v2 Offset, const char* Format, ...);
    void DrawLine(v3 Start, v3 End);
    void DrawCameraGizmo(v3 Position, float Pitch, float Yaw);
    void DrawCameraGizmo(const mat4& ModelMatrix);
    void DrawAxisGizmo(mat4 ModelMatrix, bool ShowPos, bool ShowMatrix = false);
    void Flush(const mat4& ViewProj, int ViewportX, int ViewportY, int ViewportWidth, int ViewportHeight, int WindowWidth, int WindowHeight);

private:
    std::vector<command> Commands;
    std::string StringBuffer;
    v3 LastColor = {};
    
	GLuint Program = 0;
    GLuint VAO = 0;
    GLuint LineBuffer = 0;

    std::vector<line_vertex> VertexBuffer;
};
