#pragma once
#include <float.h>
#include <string>

#include "opengl_headers.h"
#include "mesh.h"

namespace GL
{
	struct Texture
	{
		Texture(GLenum _Flag) : Flag(_Flag) {}
		~Texture() = default;

		GLuint ID = 0;
		GLenum Flag = 0;

		void bind() { glBindTexture(Flag, ID); }
		void unbind() { glBindTexture(Flag, 0); }
	};

	struct Renderbuffer
	{
		Renderbuffer() { glGenRenderbuffers(1, &ID); }
		~Renderbuffer() { glDeleteRenderbuffers(1, &ID); }

		GLuint ID = 0;

		void bind() { glBindRenderbuffer(GL_RENDERBUFFER, ID); }
		static void unbind() { glBindRenderbuffer(GL_RENDERBUFFER, 0); }
	};

	struct VertexArrayObject
	{
		VertexArrayObject() { glGenVertexArrays(1, &ID); }
		~VertexArrayObject() { glDeleteVertexArrays(1, &ID); }

		GLuint ID = 0;

		void bind() { glBindVertexArray(ID); }
		static void unbind() { glBindVertexArray(0); }
	};

	struct VertexBufferObject
	{
		VertexBufferObject() = default;
		~VertexBufferObject() = default;

		GLuint ID = 0;

		void bind() { glBindBuffer(GL_ARRAY_BUFFER, ID); }
		static void unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }
	};

	struct Framebuffer
	{
		Framebuffer() { glGenFramebuffers(1, &ID); }
		~Framebuffer() { glDeleteFramebuffers(1, &ID); }

		GLuint ID = 0;
		Texture ColorTexture = Texture(GL_TEXTURE_2D);
		Renderbuffer DepthStencilRenderbuffer;

		void bind() { glBindFramebuffer(GL_FRAMEBUFFER, ID);  }
		static void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
	};

	struct Program
	{
		Program() = default;
		~Program() { glDeleteProgram(ID); }

		GLuint ID = 0;

		void bind() { glUseProgram(ID); }
		static void unbind() { glUseProgram(0); }
	};
}

namespace RBS
{
	struct Mesh
	{
		Mesh();
		~Mesh() {}

		GL::VertexArrayObject	VAO;
		GL::VertexBufferObject	VBO;

		vertex_descriptor	Descriptor;

		int VertexCount = 0;

		void CreateBufferData(const void* data);
		void CreateVertexArray();

		void Draw();
	};

	struct CubeMap
	{
		Mesh CubeMesh;

		GL::Program Program;
		GL::VertexArrayObject VAO;
		GL::VertexBufferObject VBO;
		GL::Texture Texture = GL::Texture(GL_TEXTURE_CUBE_MAP);

		void Create(const std::string facesStr[6]);
		void Draw(const mat4& ProjectionMatrix, const mat4& ViewMatrix);

	private:
		void GenerateTextures(const std::string facesStr[6]);
		void BuildCube();
	};
}