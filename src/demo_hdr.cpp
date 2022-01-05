
#include <vector>

#include <imgui.h>
#include <iostream>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_hdr.h"

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

layout (location = 1) out vec4 BrightColor;

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

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
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
    vec3 result = (ambientColor + diffuseColor + specularColor + emissiveColor);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    // Apply light color
    oColor = vec4(result, 1.0);
})GLSL";

static const char* gVertexShaderHDRStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Varyings
out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = vec4(aPosition, 1.0);

})GLSL";

static const char* gFragmentShaderHDRStr = R"GLSL(
// Varyings
in vec2 vUV;

uniform float explosure;

uniform bool hdr;
uniform bool bloom;

// Uniforms
uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;

// Shader outputs
out vec4 oColor;

void main()
{             
    vec3 pureColor = texture(hdrBuffer, vUV).rgb;
    vec3 bloomColor = texture(bloomBlur, vUV).rgb;

    if(bloom)
       pureColor += bloomColor; // additive blending

    vec3 hdrColor = pureColor;
    const float gamma = 2.2;


    
    vec3 result;
    
    if (hdr)
    {
        hdrColor = vec3(1.0) - exp(-pureColor * explosure);     
    }
    
    result = pow(hdrColor, vec3(1.0/gamma));
    oColor = vec4(result, 1.0);
} 
)GLSL";

static const char* gVertexShaderBloomStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Varyings
out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = vec4(aPosition, 1.0);

})GLSL";

static const char* gFragmentShaderBloomStr = R"GLSL(
// Varyings
in vec2 vUV;

// Uniforms
uniform sampler2D brightImage;

uniform bool horizontal;
uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

// Shader outputs
out vec4 oColor;

void main()
{             
    vec2 tex_offset = 1.0 / textureSize(brightImage, 0); // gets size of single texel
    vec3 result = texture(brightImage, vUV).rgb * weight[0];

    float xoffset = float(horizontal) * tex_offset.x;
    float yoffset = float(!horizontal) * tex_offset.y;

    for(int i = 1; i < 5; ++i)
    {
        vec2 uvoffset = i * vec2(xoffset, yoffset);

        result += texture(brightImage, vUV + uvoffset).rgb * weight[i];
        result += texture(brightImage, vUV - uvoffset).rgb * weight[i];
    }

     oColor = vec4(result, 1.0);
} 
)GLSL";

demo_hdr::demo_hdr(const platform_io& IO, GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), TavernScene(GLCache)
{
    #pragma region LitShader
    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        this->Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, true);
    }

    // Create a vertex array and bind attribs onto the vertex buffer
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, TavernScene.MeshBuffer);

        vertex_descriptor& Desc = TavernScene.MeshDesc;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.NormalOffset);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }

    #pragma endregion

#pragma region HDRShader

    // Create shader
    {
        this->ProgramHDR = GL::CreateProgramEx(1, &gVertexShaderHDRStr, 1, &gFragmentShaderHDRStr, true);
    }

    // Create shader
    {
        this->ProgramBloom = GL::CreateProgramEx(1, &gVertexShaderBloomStr, 1, &gFragmentShaderBloomStr, true);
    }
    // set up floating point framebuffer to render scene to
    {
        glGenFramebuffers(1, &hdrFBO);

        // create floating point color buffer
        glGenTextures(2, colorBuffers);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, IO.WindowWidth, IO.WindowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glBindTexture(GL_TEXTURE_2D, 0);


        // create depth buffer (renderbuffer)
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, IO.WindowWidth, IO.WindowHeight);

        // attach buffers
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffers[0], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, colorBuffers[1], 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        glGenFramebuffers(2, pingpongFBO);
        glGenTextures(2, pingpongColorbuffers);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, IO.WindowWidth, IO.WindowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
            // also check if framebuffers are complete (no need for depth buffer)
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "Framebuffer not complete!" << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glUseProgram(ProgramHDR);
        glUniform1i(glGetUniformLocation(ProgramHDR, "hdrBuffer"), 0);
        glUniform1i(glGetUniformLocation(ProgramHDR, "bloomBlur"), 1);

    }

    // Gen mesh
    {
        // Create a descriptor based on the `struct vertex` format
        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex, Position);
        Descriptor.UVOffset = OFFSETOF(vertex, UV);

        // Create a cube in RAM
        vertex Quad[6];
        this->vertexCount = 6;
        Mesh::BuildScreenQuad(Quad, Quad + this->vertexCount, Descriptor, {2.f,2.f});

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &this->vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, this->vertexCount * sizeof(vertex), Quad, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &quadVAO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, this->vertexBuffer);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, Position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)OFFSETOF(vertex, UV));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

#pragma endregion

}

demo_hdr::~demo_hdr()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

static void DrawQuad(GLuint Program, mat4 ModelViewProj)
{
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelViewProj"), 1, GL_FALSE, ModelViewProj.e);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void demo_hdr::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);
    
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    // Render tavern
    this->RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //Bloom
    bool horizontal = true;
    glUseProgram(ProgramBloom);
    for (unsigned int i = 0; i < bloomIteration; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        glUniform1i(glGetUniformLocation(ProgramBloom, "horizontal"), horizontal);
        glBindTexture(GL_TEXTURE_2D, i == 0 ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
        RenderHdrTavern(ProjectionMatrix * ViewMatrix * ModelMatrix);
        horizontal = !horizontal;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

   
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ProgramHDR);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);

    glUniform1i(glGetUniformLocation(ProgramHDR, "hdr"), hdr);
    glUniform1i(glGetUniformLocation(ProgramHDR, "bloom"), bloomIteration > 0);
    glUniform1f(glGetUniformLocation(ProgramHDR, "explosure"), explosure);
   //HDR
   this->RenderHdrTavern(ProjectionMatrix * ViewMatrix * ModelMatrix);

    // Render tavern wireframe

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_hdr::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_hdr", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("Wireframe", &Wireframe);
        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        TavernScene.InspectLights();

        if (ImGui::TreeNodeEx("Post Process"))
        {
            ImGui::DragInt("Bloom Iteration", &bloomIteration, 1.0f, 0, 50);
            ImGui::Checkbox("Hdr", &hdr);
            if (hdr)
            {
                if (ImGui::TreeNodeEx("Hdr Settings"))
                {
                    ImGui::SliderFloat("explosure", &explosure, 0, 1);
                    ImGui::TreePop();

                }
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

void demo_hdr::RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TavernScene.DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TavernScene.EmissiveTexture);
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);
    glBindVertexArray(0);
}

void demo_hdr::RenderHdrTavern(const mat4& modelViewProj)
{
    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(quadVAO);
    DrawQuad(ProgramHDR, modelViewProj);
    glBindVertexArray(0);
   
}
