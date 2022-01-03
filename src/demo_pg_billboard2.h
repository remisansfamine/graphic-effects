#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "pg_debug_renderer.h"

class demo_pg_billboard2 : public demo
{
public:
    demo_pg_billboard2();
    virtual ~demo_pg_billboard2();
    virtual void Update(const platform_io& IO);

private:
    // 3d camera
    camera Camera = {};
    camera CameraDebug = {};
    bool CameraDebugUse = false;
    bool CameraDebugControl = false;
    bool CameraDebugLookAt = false;

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint Texture = 0;

    GLuint VAO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

};
