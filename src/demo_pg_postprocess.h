#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "demo_base.h"

enum class color_transform : int
{
    IDENTITY,
    GRAYSCALE,
    SEPIA,
    INVERSE,
    CUSTOM
};

class demo_pg_postprocess : public demo
{
public:
    // New framebuffer to render offscreen
    struct framebuffer
    {
        GLuint FBO;
        GLuint ColorTexture;
        GLuint DepthStencilRenderbuffer;
    };

    // First pass is the render inside demo_base::Update()
    // Second pass data (color transformation)
    struct postprocess_pass_data
    {
        GLuint Program = 0;
        GLuint VAO = 0;
        GLuint VertexBuffer = 0; // We will store a quad (6 vertices)
    };

    demo_pg_postprocess(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_pg_postprocess();
    virtual void Update(const platform_io& IO);
    
    color_transform ColorTransformMode = color_transform::GRAYSCALE;

private:
    // 3d camera
    camera Camera = {};

    demo_base DemoBase;
    postprocess_pass_data PostProcessPassData = {};
    framebuffer Framebuffer = {};
};
