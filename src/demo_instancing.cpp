
#include <vector>

#include <imgui.h>
#include <iostream>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_instancing.h"

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
static const char* gVertexShaderStr = R"GLSL(
#version 330 core

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in mat4 aModel;

uniform mat4  uViewProj;

// Varyings (variables that are passed to fragment shader with perspective interpolation)
out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = uViewProj * aModel * vec4(aPosition, 1.0);
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
#version 330 core

// Varyings
in vec2 vUV;

// Uniforms
uniform sampler2D uColorTexture;

// Shader outputs
out vec4 oColor;

void main()
{
    oColor = texture(uColorTexture, vUV);
})GLSL";

demo_instancing::demo_instancing(GL::cache& GLCache, GL::debug& GLDebug)
    : GLCache(GLCache), GLDebug(GLDebug)
{
    // Create render pipeline
    this->Program = GL::CreateProgram(gVertexShaderStr, gFragmentShaderStr);
    
    int amount = 100000;
    float radius = 150.f;
    for (unsigned int i = 0; i < amount; i++)
    {
        float randX = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f;
        float randY = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f;
        float randZ = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f;

        float randRadius = (((float)rand() / (float)RAND_MAX) + 0.5f) * radius;

        v3 randVec = { randX, randY, randZ };
        v3 superVec = Vec3::Normalize(randVec) * randRadius;

        float xRot = ((float)rand() / (float)RAND_MAX) * Math::TwoPi();
        float yRot = ((float)rand() / (float)RAND_MAX) * Math::TwoPi();
        float zRot = ((float)rand() / (float)RAND_MAX) * Math::TwoPi();

        offsets.push_back(Mat4::Translate(superVec) * Mat4::RotateX(xRot) * Mat4::RotateY(yRot) * Mat4::RotateZ(zRot));
    }

    // Gen obj
    {
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);

        // Load obj and get the VBO
        VertexBuffer = GLCache.LoadObj("media/rock.obj", 1.f, &VertexCount);

        // Create a vertex array
        glGenVertexArrays(1, &VAO);

        // Generate VBO
        glBindVertexArray(VAO);
        {
            glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.PositionOffset));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.UVOffset));

            // Gen instance VBO
            glGenBuffers(1, &instanceVBO);
            {
                glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(mat4) * offsets.size(), offsets.data(), GL_STATIC_DRAW);

                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(0 * sizeof(v4)));
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(1 * sizeof(v4)));
                glEnableVertexAttribArray(4);
                glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(2 * sizeof(v4)));
                glEnableVertexAttribArray(5);
                glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(3 * sizeof(v4)));
            }

            glVertexAttribDivisor(2, 1);
            glVertexAttribDivisor(3, 1);
            glVertexAttribDivisor(4, 1);
            glVertexAttribDivisor(5, 1);
        }
        glBindVertexArray(0);
        //glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Gen texture
    {
        Texture = GLCache.LoadTexture("media/rock.png", IMG_GEN_MIPMAPS);
    }
}

demo_instancing::~demo_instancing()
{
    // Cleanup GL
    glDeleteTextures(1, &Texture);
    glDeleteBuffers(1, &VertexBuffer);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteProgram(Program);
}

void demo_instancing::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_instancing", ImGuiTreeNodeFlags_Framed))
    {
        static bool swag = false;
        ImGui::Checkbox("Are you swag ?", &swag);
        ImGui::TreePop();
    }
}

void demo_instancing::Update(const platform_io& IO)
{
    mat4 rotate = Mat4::RotateX(IO.DeltaTime);

    for (int i = 0; i < offsets.size(); i++)
        offsets[i] *= rotate;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, offsets.size() * sizeof(mat4), offsets.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Compute model-view-proj and send it to shader
    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), (float)IO.WindowWidth / (float)IO.WindowHeight, 0.1f, 1000.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    
    // Setup GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Clear screen
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use shader and send data
    glUseProgram(Program);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);

    // Draw origin
    PG::DebugRenderer()->DrawAxisGizmo(Mat4::Translate({ 0.f, 0.f, 0.f }), true, false);

    mat4 ViewProj = ProjectionMatrix * ViewMatrix;

    glUniformMatrix4fv(glGetUniformLocation(Program, "uViewProj"), 1, GL_FALSE, ViewProj.e);

    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, VertexCount, offsets.size());
    glBindVertexArray(0);

    //DisplayDebugUI();
}
