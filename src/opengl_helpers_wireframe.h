#pragma once

#include <vector>

#include "maths.h"

#include "opengl_headers.h"

namespace GL
{
	class wireframe_renderer
	{
	public:
		wireframe_renderer();
		~wireframe_renderer();

		void BindBuffer(GLuint MeshVBO, GLsizei PositionStride, GLsizei PositionOffset, int VertexCount);
		void DrawArray(GLint First, GLsizei Count, const mat4& MVP);
		void Flush();

	private:	
		enum class command_type
		{
			BIND_BUFFER,
			DRAW_ARRAY
		};

		struct cmd_bind_buffer
		{
			GLuint MeshVBO;
			GLsizei PositionStride;
			GLsizei PositionOffset;
			int VertexCount;
		};
		
		struct cmd_draw_array
		{
			GLint First;
			GLsizei Count;
			mat4 MVP;
		};

		struct command
		{
			command_type Type;
			union
			{
				cmd_bind_buffer BindBuffer;
				cmd_draw_array DrawArray;
			};
		};
	
		void SendBindBuffer(const cmd_bind_buffer& Cmd);
		void SendDrawArray(const cmd_draw_array& Cmd);

		GLuint Program = 0;
		GLuint VAO = 0;
		std::vector<v3> BaryBufferData;
		GLuint BaryBuffer = 0;
		std::vector<command> Commands;
	};
}