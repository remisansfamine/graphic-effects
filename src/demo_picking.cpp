
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_picking.h"

#include "pg.h"

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
    vec3 emissiveColor = gDefaultMaterial.emission;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";

static const char* gVertexShaderPickStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;

// Uniforms
uniform mat4 uViewProjection;
uniform mat4 uModel;

void main()
{
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
})GLSL";

static const char* gFragmentShaderPickStr = R"GLSL(
out vec4 oColor;

uniform vec4 uPickingColor;

void main()
{
    oColor = uPickingColor;
})GLSL";

demo_picking::demo_picking(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : GLCache(GLCache), GLDebug(GLDebug), TavernScene(GLCache)
{
    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }


    // Create render pipeline
    Picking.Program = GL::CreateProgram(gVertexShaderPickStr, gFragmentShaderPickStr);

    // Gen mesh
    {
        Model modelBasic;

        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.HasNormal = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.NormalOffset = OFFSETOF(vertex_full, Normal);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);

        modelBasic.VBO = GLCache.LoadObj("media/backpack.obj", 1.f, &modelBasic.VertexCount);
        modelBasic.Texture = GLCache.LoadTexture("media/diffuse.jpg", IMG_GEN_MIPMAPS);

        // Create a vertex array
        glGenVertexArrays(1, &modelBasic.VAO);
        glBindVertexArray(modelBasic.VAO);
        {
            //glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.PositionOffset));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.UVOffset));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, (void*)(Descriptor.NormalOffset));

            glEnableVertexAttribArray(0);
        }
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Generate FBO for outline
        {
            // Geneate the framebuffer and bind it
            glGenFramebuffers(1, &Picking.FBO);
            glBindFramebuffer(GL_FRAMEBUFFER, Picking.FBO);

            // Generate the color attachement texture
            {
                glGenTextures(1, &Picking.Texture);
                glBindTexture(GL_TEXTURE_2D, Picking.Texture);

                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, IO.WindowWidth, IO.WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Picking.Texture, 0);

                    //glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        float offset = 4.f;
        int id = 1;
        for (int x = 0; x < 10; x++)
        {
            for (int y = 0; y < 10; y++)
            {
                modelBasic.position = { (float)x * offset, (float)y * offset, -20.f };
                modelBasic.ID.r = (id & 0x000000FF) >> 0;
                modelBasic.ID.g = (id & 0x0000FF00) >> 8;
                modelBasic.ID.b = (id & 0x00FF0000) >> 16;

                models.push_back(modelBasic);

                id++;
            }
        }
    }
}

demo_picking::~demo_picking()
{
    // Cleanup GL
    for (auto& model : models)
    {
        glDeleteBuffers(1, &model.VBO);
        glDeleteVertexArrays(1, &model.VAO);
    }

    {
        glDeleteFramebuffers(1, &Picking.FBO);
        glDeleteFramebuffers(1, &Picking.FBO);
        glDeleteTextures(1, &Picking.Texture);
        glDeleteProgram(Picking.Program);
    }
    
    glDeleteProgram(Program);

    models.clear();
}

void demo_picking::DisplayDebugUI(const platform_io& IO)
{
    if (ImGui::TreeNodeEx("demo_picking", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        TavernScene.InspectLights();

        const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
        ImGui::Image((ImTextureID)Picking.Texture, { 256 * AspectRatio, 256 }, { 0,1 }, {1,0});

        ImGui::TreePop();
    }
}

void demo_picking::RenderOutline(const mat4& ModelViewProj)
{
    // Change color for depth test for render quad
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glUseProgram(OutlineProgram);
    //glBindVertexArray(QuadVAO);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, OutlineTexture);

    //glUniformMatrix4fv(glGetUniformLocation(OutlineProgram, "uModelViewProj"), 1, GL_FALSE, ModelViewProj.e);
    //glUniform2fv(glGetUniformLocation(OutlineProgram, "uSmooth"), 1, smoothStep.e);
    //glUniform3fv(glGetUniformLocation(OutlineProgram, "uEdgeColor"), 1, edgeColor.e);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void demo_picking::RenderPickingTexture(const mat4 ViewProj)
{
    glUseProgram(Picking.Program);

    glUniformMatrix4fv(glGetUniformLocation(Picking.Program, "uViewProjection"), 1, GL_FALSE, ViewProj.e);

    glBindFramebuffer(GL_FRAMEBUFFER, Picking.FBO);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto& model : models)
    {
        mat4 ModelMatrix = Mat4::Translate(model.position);

        // Bind uniform buffer and textures
        glUniformMatrix4fv(glGetUniformLocation(Picking.Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
        glUniform4f(glGetUniformLocation(Picking.Program, "uPickingColor"), (float)model.ID.r / 255.f, (float)model.ID.g / 255.f, (float)model.ID.b / 255.f, 1.0f);

        // Draw mesh
        glBindVertexArray(model.VAO);
        glDrawArrays(GL_TRIANGLES, 0, model.VertexCount);
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#include <iostream>
void demo_picking::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);

    for (int i = 0; i < models.size(); i++)
    {
        if (Picking.PickedID != -1 && i == Picking.PickedID)
            continue;

        mat4 ModelMatrix = Mat4::Translate(models[i].position);
        mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));

        // Bind uniform buffer and textures
        glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
        glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);

        glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, models[i].Texture);
        glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

        // Draw mesh
        glBindVertexArray(models[i].VAO);
        glDrawArrays(GL_TRIANGLES, 0, models[i].VertexCount);
    }

    //RenderPickingTexture(ProjectionMatrix * ViewMatrix);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        RenderPickingTexture(ProjectionMatrix * ViewMatrix);

        ImVec2 mousePos = ImGui::GetMousePos();

        glFlush(); // Send waiting commands (draw, etc)
        glFinish(); // Wait for commands to be done

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        unsigned char data[4];
        glReadPixels((GLint)mousePos.x, IO.WindowHeight - (GLint)mousePos.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

        int pickedID = data[0] + data[1] * 256 + data[2] * 256 * 256;
        std::cout << "ID " << pickedID << std::endl;
        std::cout << "Mouse Pos : X = " << mousePos.x << " , Y = " << mousePos.y << std::endl;

        Picking.PickedID = pickedID - 1;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    glDisable(GL_DEPTH_TEST);

    DisplayDebugUI(IO);
}
