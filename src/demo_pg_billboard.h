#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "demo_base.h"

enum billboard_properties
{
    BIL_VERTICAL_AXIS_ALIGNED = 1 << 0,
    BIL_VIEW_PLANE_ALIGNED = 1 << 1
};

class demo_pg_billboard : public demo
{
public:
    demo_pg_billboard(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_pg_billboard();
    virtual void Update(const platform_io& IO);

private:
    // 3d camera
    camera Camera = {};
    camera CameraDebug = {};
    bool CameraDebugUse = false;
    bool CameraDebugControl = false;
    bool CameraDebugLookAt = false;

    demo_base DemoBase;

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint Texture = 0;

    GLuint VAO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

    unsigned int BillboardProperties = 0;
};
