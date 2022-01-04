#pragma once

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "demo_base.h"

class demo_fbo : public demo
{
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
        GLuint VertexCount = 6; // We will store a quad (6 vertices)
    };

public:
    demo_fbo(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_fbo();
    virtual void Update(const platform_io& IO);

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};
    
    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;

    postprocess_pass_data PostProcessPassData;
    framebuffer Framebuffer;

    demo_base DemoBase;
};
