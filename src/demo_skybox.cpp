
#include <vector>

#include <imgui.h>
#include <stb_image.h>

#include "opengl_helpers.h"
#include "maths.h"
#include "mesh.h"
#include "color.h"

#include "demo_skybox.h"

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
static const char* gPPVertShaderStr = R"GLSL(
layout (location = 0) in vec3 vertPos;

out vec3 TexCoords;

uniform mat4 uViewProj;

void main()
{
    TexCoords = vertPos;
    gl_Position = uViewProj * vec4(vertPos, 1.0);
})GLSL";

static const char* gPPFragShaderStr = R"GLSL(
out vec4 FragColor;
in  vec3 TexCoords;

uniform samplerCube skyTexture;

void main()
{    
    FragColor = texture(skyTexture, TexCoords);
})GLSL";

// Shaders
// ==================================================
static const char* gPPVertShaderBoxStr = R"GLSL(
layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec3 vertNormal;

out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
 
void main()
{
    Normal = mat3(transpose(inverse(model))) * vertNormal;
    Position = vec3(model * vec4(vertPos, 1.0));
    gl_Position = projection * view * vec4(Position, 1.0);
})GLSL";

static const char* gPPFragShaderBoxStr = R"GLSL(
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform int onReflect;
uniform float refractRatio;
uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{             
    vec3 I = normalize(Position - cameraPos);

    vec3 R;
    if (onReflect > 0)
        R = reflect(I, normalize(Normal));
    else
        R = refract(I, normalize(Normal), refractRatio);

    FragColor = vec4(texture(skybox, R).rgb, 1.0);
})GLSL";

static const std::string skyboxFaces[6] = {
    "media\\right.jpg",
    "media\\left.jpg",
    "media\\top.jpg",
    "media\\bottom.jpg",
    "media\\front.jpg",
    "media\\back.jpg"
};

#include <iostream>
demo_skybox::demo_skybox(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), DemoBase(GLCache, GLDebug)
{
    // Generate ID texture
    glGenTextures(1, &Skybox.ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox.ID);

    stbi_set_flip_vertically_on_load(false);

    int channel;

    // Load and generate skybox faces
    for (unsigned int i = 0; i < 6; i++)
    {
        int dimX, dimY;
        unsigned char* data = stbi_load(skyboxFaces[i].c_str(), &dimX, &dimY, &channel, 4);

        if (!data)
        {
            // Error on load (missing textures or fail open file)
            const char* error = stbi_failure_reason();
            std::string errorStr = error + ' ';

            std::cout << error + skyboxFaces[i] << std::endl;
            continue;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, dimX, dimY, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    // Set textures parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Gen cube and its program
    {
        Skybox.Program = GL::CreateProgram(gPPVertShaderStr, gPPFragShaderStr);

        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex, Position);
        Descriptor.UVOffset = OFFSETOF(vertex, UV);

        // Create cube vertices
        vertex Cube[36];
        Skybox.VertexCount = 36;
        Mesh::BuildCube(Cube, Cube + Skybox.VertexCount, Descriptor);

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &Skybox.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, Skybox.VBO);
        glBufferData(GL_ARRAY_BUFFER, Skybox.VertexCount * sizeof(vertex), Cube, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &Skybox.VAO);
        glBindVertexArray(Skybox.VAO);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(Descriptor.PositionOffset));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Gen cube and its program
    {
        Program = GL::CreateProgram(gPPVertShaderBoxStr, gPPFragShaderBoxStr);

        vertex_descriptor Descriptor = {};
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.NormalOffset = OFFSETOF(vertex_full, Normal);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);

        // Create cube vertices
        /*const int lon = 20;
        const int lat = 20;
        VertexCount = lon * lat * 6;

        vertex_full Sphere[lon * lat * 6];
        Mesh::BuildSphere(Sphere, Sphere + VertexCount, Descriptor, lon, lat);*/

        VertexBuffer = GLCache.LoadObj("media/backpack.obj", 1.f, &VertexCount);

        // Upload cube to gpu (VRAM)
        //glGenBuffers(1, &VertexBuffer);
        //glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
        //glBufferData(GL_ARRAY_BUFFER, VertexCount * sizeof(vertex_full), Sphere, GL_STATIC_DRAW);

        // Create a vertex array
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)(Descriptor.PositionOffset));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_full), (void*)(Descriptor.NormalOffset));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

  demo_skybox::~demo_skybox()
{
    // Cleanup GL
    if (Skybox.ID)
        glDeleteTextures(1, &Skybox.ID);
    if (Skybox.VBO)
        glDeleteBuffers(1, &Skybox.VBO);
    if (Skybox.VAO)
        glDeleteVertexArrays(1, &Skybox.VAO);
    if (Skybox.Program)
        glDeleteProgram(Skybox.Program);

    if (VertexBuffer)
        glDeleteBuffers(1, &VertexBuffer);
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (Program)
        glDeleteProgram(Program);
}

void demo_skybox::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_skybox", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        static bool onReflect = true;
        if (ImGui::Checkbox("Reflection", &onReflect))
            reflect = (int)onReflect;

        if (!reflect)
        {
            bool test = false;
            static int e = 0;
            test |= ImGui::RadioButton("Air", &e, 0);
            test |= ImGui::RadioButton("Water", &e, 1);
            test |= ImGui::RadioButton("Ice", &e, 2);
            test |= ImGui::RadioButton("Glass", &e, 3);
            test |= ImGui::RadioButton("Diamond", &e, 4);

            if (test)
            {
                switch (e)
                {
                case 0:
                    refractRatio = 1.00f;
                    break;
                case 1:
                    refractRatio = 1.33f;
                    break;
                case 2:
                    refractRatio = 1.309f;
                    break;
                case 3:
                    refractRatio = 1.52f;
                    break;
                case 4:
                    refractRatio = 2.42f;
                    break;
                default:
                    refractRatio = 1.00f;
                    break;
                }
            }
        }

        ImGui::TreePop();
    }
}

void demo_skybox::Update(const platform_io& IO)
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthMask(0);

    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    mat4 CastView = mat4(Mat4::Mat4(Mat3::Mat3(CameraGetInverseMatrix(Camera))));
    mat4 VPMatrix = ProjectionMatrix * CastView;

    // Bind shader program
    glUseProgram(Skybox.Program);

    // Set uniforms
    //int skyId = 0;
    //glUniform1iv(glGetUniformLocation(Program, "skyTexture"), 1, &skyId);
    glUniformMatrix4fv(glGetUniformLocation(Skybox.Program, "uViewProj"), 1, GL_FALSE, VPMatrix.e);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox.ID);

    // Draw
    glBindVertexArray(Skybox.VAO);
    glDrawArrays(GL_TRIANGLES, 0, Skybox.VertexCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glDepthMask(1);


    glEnable(GL_DEPTH_TEST);
    // Draw reflect/refract box
    glUseProgram(Program);

    v3 pos = { 0.f, 0.f, -15.f };
    mat4 model = Mat4::Translate(pos);
    glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, model.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, CameraGetInverseMatrix(Camera).e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "projection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "cameraPos"), 1, Camera.Position.e);
    glUniform1iv(glGetUniformLocation(Program, "onReflect"), 1, &reflect);
    glUniform1fv(glGetUniformLocation(Program, "refractRatio"), 1, &refractRatio);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox.ID);

    // Draw
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, VertexCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glDisable(GL_DEPTH_TEST);
    
    //DemoBase.RenderTavern(ProjectionMatrix, CameraGetInverseMatrix(Camera), Mat4::Identity());

    DisplayDebugUI();
}