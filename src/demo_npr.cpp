
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_npr.h"

#include "pg.h"

demo_npr::demo_npr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : GLCache(GLCache), GLDebug(GLDebug)
{
    // Create render pipeline
    Program = GL::CreateProgramFromFiles("src/shaders/toon_shader.vert", "src/shaders/toon_shader.frag");

    // Gen mesh
    {
        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.HasNormal = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.NormalOffset = OFFSETOF(vertex_full, Normal);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);

        VertexBuffer = GLCache.LoadObj("media/T-Rex.obj", 1.f, &VertexCount);

        // Create a vertex array
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        {
            glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.PositionOffset));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.UVOffset));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.NormalOffset));

            glEnableVertexAttribArray(0);
        }
        glBindVertexArray(0);
        //glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Gen and bind palette texture
    {

        Texture = GLCache.LoadTexture("media/ToonPalette.png", IMG_GEN_MIPMAPS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glUseProgram(Program);

        glUniform1i(glGetUniformLocation(Program, "uToonPalette"), 0);
        glUniform1i(glGetUniformLocation(Program, "uUsePalette"), (int)usePalette);

        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    // Generate FBO for outline
    {
        // Geneate the framebuffer and bind it
        glGenFramebuffers(1, &OutlineFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, OutlineFBO);

        // Generate the color attachement texture
        {
            glGenTextures(1, &OutlineTexture);
            glBindTexture(GL_TEXTURE_2D, OutlineTexture);

            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, IO.WindowWidth, IO.WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, OutlineTexture, 0);

                //glBindTexture(GL_TEXTURE_2D, 0);
            }

            // Geneate the renderbuffer and bind it to the framebuffer
            {
                glGenRenderbuffers(1, &RenderBuffer);
                glBindRenderbuffer(GL_RENDERBUFFER, RenderBuffer);

                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, IO.WindowWidth, IO.WindowHeight);

                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RenderBuffer);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        OutlineProgram = GL::CreateProgramFromFiles("src/shaders/outline_shader.vert", "src/shaders/outline_shader.frag");

        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);

        // Create a cube in RAM
        vertex_full Quad[6];
        Mesh::BuildScreenQuad(Quad, Quad + 6, Descriptor, { 2.f, 2.f });

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &QuadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vertex_full), Quad, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &QuadVAO);
        glBindVertexArray(QuadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)Descriptor.PositionOffset);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)Descriptor.UVOffset);

        //glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

demo_npr::~demo_npr()
{
    // Cleanup GL
    glDeleteFramebuffers(1, &OutlineFBO);
    glDeleteBuffers(1, &QuadVBO);
    glDeleteBuffers(1, &VertexBuffer);
    glDeleteVertexArrays(1, &QuadVAO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &OutlineTexture);
    glDeleteProgram(OutlineProgram);
    glDeleteProgram(Program);
}

void demo_npr::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_npr", ImGuiTreeNodeFlags_Framed))
    {
        ImGui::DragFloat4("LightPos", lightPos.e, 0.1f);
        ImGui::ColorEdit3("Color", color.e);

        if (ImGui::Checkbox("Use palette texture", &usePalette))
        {
            glUseProgram(Program);
            glUniform1i(glGetUniformLocation(Program, "uUsePalette"), (int)usePalette);
            glUseProgram(0);
        }

        ImGui::Spacing();

        ImGui::ColorEdit3("EdgeColor", edgeColor.e);
        ImGui::DragFloat2("SmoothStep", smoothStep.e, 0.01f);

        ImGui::TreePop();
    }
}

void demo_npr::RenderOutline(const mat4& ModelViewProj)
{
    // Change color for depth test for render quad
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(OutlineProgram);
    glBindVertexArray(QuadVAO);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, OutlineTexture);

    glUniformMatrix4fv(glGetUniformLocation(OutlineProgram, "uModelViewProj"), 1, GL_FALSE, ModelViewProj.e);
    glUniform2fv(glGetUniformLocation(OutlineProgram, "uSmooth"), 1, smoothStep.e);
    glUniform3fv(glGetUniformLocation(OutlineProgram, "uEdgeColor"), 1, edgeColor.e);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void demo_npr::Update(const platform_io& IO)
{
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Compute model-view-proj and send it to shader
    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), (float)IO.WindowWidth / (float)IO.WindowHeight, 0.1f, 1000.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));

    glBindFramebuffer(GL_FRAMEBUFFER, OutlineFBO);

    // Setup GL state
    glEnable(GL_DEPTH_TEST);

    // Clear screen
    glClearColor(0.05f, 0.05f, 0.05f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(Program);

    // Use shader and send data
    glUniform4fv(glGetUniformLocation(Program, "uLightPos"), 1, lightPos.e);
    glUniform3fv(glGetUniformLocation(Program, "uColor"), 1, color.e);
    //glUniform3fv(glGetUniformLocation(Program, "uViewDir"), 1, Camera.Position.e);

    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, VertexCount);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glUseProgram(0);

    RenderOutline(Mat4::Identity());

    DisplayDebugUI();
}
