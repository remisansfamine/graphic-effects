#include "structures.h"

#include <stb_image.h>

#include "opengl_helpers.h"
#include "platform.h"
#include "mesh.h"

namespace RBS
{
    Mesh::Mesh()
    {
        // Create a descriptor based on the `struct vertex` format
        Descriptor.Stride = sizeof(vertex_full);
        Descriptor.HasUV = true;
        Descriptor.HasNormal = true;
        Descriptor.PositionOffset = OFFSETOF(vertex_full, Position);
        Descriptor.UVOffset = OFFSETOF(vertex_full, UV);
        Descriptor.NormalOffset = OFFSETOF(vertex_full, Normal);
        Descriptor.TangentOffset = OFFSETOF(vertex_full, Tangent);
        Descriptor.BitangentOffset = OFFSETOF(vertex_full, Bitangent);
    }

	void Mesh::CreateBufferData(const void* data)
	{
		VBO.bind();
		glBufferData(GL_ARRAY_BUFFER, VertexCount * Descriptor.Stride, data, GL_STATIC_DRAW);
	}

	void Mesh::CreateVertexArray()
	{
        VAO.bind();
        VBO.bind();

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, reinterpret_cast<void*>(Descriptor.PositionOffset));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Descriptor.Stride, reinterpret_cast<void*>(Descriptor.UVOffset));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, reinterpret_cast<void*>(Descriptor.NormalOffset));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, reinterpret_cast<void*>(Descriptor.TangentOffset));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, Descriptor.Stride, reinterpret_cast<void*>(Descriptor.BitangentOffset));
	}

    void Mesh::Draw()
    {
        VAO.bind();
        glDrawArrays(GL_TRIANGLES, 0, VertexCount);
        VAO.unbind();
    }




    void CubeMap::GenerateTextures(const std::string facesStr[6])
    {
        // Generate ID texture
        glGenTextures(1, &Texture.ID);
        Texture.bind();

        stbi_set_flip_vertically_on_load(false);

        int channel;

        // Load and generate skybox faces
        for (unsigned int i = 0; i < 6; i++)
        {
            int dimX, dimY;
            unsigned char* data = stbi_load(facesStr[i].c_str(), &dimX, &dimY, &channel, 4);

            if (!data)
            {
                // Error on load (missing textures or fail open file)
                const char* error = stbi_failure_reason();
                std::string errorStr = error + ' ' + facesStr[i];

                fprintf(stderr, "Unable to load cube map texture %s\n", errorStr.c_str());
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
        Texture.unbind();
    }

    void CubeMap::BuildCube()
    {
        // Create a cube in RAM
        vertex_full Cube[36];
        CubeMesh.VertexCount = 36;
        ::Mesh::BuildCube(Cube, Cube + CubeMesh.VertexCount, CubeMesh.Descriptor);

        // Upload cube to gpu (VRAM)
        glGenBuffers(1, &CubeMesh.VBO.ID);
        CubeMesh.CreateBufferData(Cube);
        CubeMesh.CreateVertexArray();
    }

    void CubeMap::Create(const std::string facesStr[6])
    {
        Program.ID = GL::CreateProgramFromFiles("src/shaders/skybox_shader.vert", "src/shaders/skybox_shader.frag");
        GenerateTextures(facesStr);
        BuildCube();

        // Set uniform on start
        Program.bind();
        glUniform1i(glGetUniformLocation(Program.ID, "uSkyTexture"), 0);
        Program.unbind();
    }

    void CubeMap::Draw(const mat4& ProjectionMatrix, const mat4& ViewMatrix)
    {
        glCullFace(GL_FRONT);

        glDepthMask(0);

        mat4 CastView = mat4(Mat4::Mat4(Mat3::Mat3(ViewMatrix)));
        mat4 VPMatrix = ProjectionMatrix * CastView;

        // Bind shader program
        Program.bind();

        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(Program.ID, "uViewProj"), 1, GL_FALSE, VPMatrix.e);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        Texture.bind();

        // Draw skybox
        CubeMesh.Draw();

        // Unbind texture
        Texture.unbind();

        glDepthMask(1);

        glCullFace(GL_BACK);
    }
}