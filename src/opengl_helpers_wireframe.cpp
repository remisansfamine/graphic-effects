
#include <cassert>

#include "opengl_helpers.h"

#include "opengl_helpers_wireframe.h"

using namespace GL;

static const char* gWireframeVertexShaderStr = R"GLSL(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aBC;
uniform mat4 uModelViewProj;
out vec3 vBC;

void main()
{
    vBC = aBC;
    gl_Position = uModelViewProj * vec4(aPosition, 1.0);
})GLSL";

static const char* gWireframeFragmentShaderStr = R"GLSL(
in vec3 vBC;
out vec4 oColor;
uniform float uLineWidth = 1.25;
uniform vec4 uLineColor = vec4(1.0, 1.0, 1.0, 0.25);

float edgeFactor()
{
    vec3 d = fwidth(vBC);
    vec3 a3 = smoothstep(vec3(0.0), d * uLineWidth, vBC);
    return min(min(a3.x, a3.y), a3.z);
}

void main()
{
    oColor = vec4(uLineColor.rgb, uLineColor.a * (1.0 - edgeFactor()));
})GLSL";

wireframe_renderer::wireframe_renderer()
{
	Program = GL::CreateProgram(gWireframeVertexShaderStr, gWireframeFragmentShaderStr);
	glGenBuffers(1, &BaryBuffer);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

wireframe_renderer::~wireframe_renderer()
{
	glDeleteProgram(Program);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &BaryBuffer);
}

void wireframe_renderer::SendBindBuffer(const wireframe_renderer::cmd_bind_buffer& Cmd)
{
	assert(Cmd.VertexCount % 3 == 0);

	// Resize barycentric coords buffer as needed
	if (Cmd.VertexCount > (int)BaryBufferData.size())
	{
		int OldSize = (int)BaryBufferData.size();
		BaryBufferData.resize(Cmd.VertexCount);
		for (int i = OldSize; i < Cmd.VertexCount; i += 3)
		{
			BaryBufferData[i + 0] = { 1.f, 0.f, 0.f };
			BaryBufferData[i + 1] = { 0.f, 1.f, 0.f };
			BaryBufferData[i + 2] = { 0.f, 0.f, 1.f };
		}
		glBindBuffer(GL_ARRAY_BUFFER, BaryBuffer);
		glBufferData(GL_ARRAY_BUFFER, BaryBufferData.size() * sizeof(v3), &BaryBufferData[0], GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}

	// Bind position buffer
	glBindBuffer(GL_ARRAY_BUFFER, Cmd.MeshVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Cmd.PositionStride, (void*)(size_t)Cmd.PositionOffset);

}

void wireframe_renderer::SendDrawArray(const wireframe_renderer::cmd_draw_array& Cmd)
{
	//glUniform1f(glGetUniformLocation(Data->WireframeShader, "uLineWidth"), LineWidth);
	//glUniform4fv(glGetUniformLocation(Data->WireframeShader, "uLineColor"), 1, LineColor.e);
	glUniformMatrix4fv(glGetUniformLocation(Program, "uModelViewProj"), 1, GL_FALSE, Cmd.MVP.e);
	glDrawArrays(GL_TRIANGLES, Cmd.First, Cmd.Count);
}

void wireframe_renderer::Flush()
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1234, -1, "Wireframe::flush");

	// Save GL state
	GLboolean PrevDepthTest;
	GLboolean PrevBlend;
	GLint PrevBlendSrcAlpha;
	glGetBooleanv(GL_DEPTH_TEST, &PrevDepthTest);
	glGetBooleanv(GL_BLEND, &PrevBlend);
	glGetIntegerv(GL_BLEND, &PrevBlendSrcAlpha);

	// Set GL state
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// Use program
	glUseProgram(Program);

	// Bind VAO
	glBindVertexArray(VAO);

	for (const command& Command : Commands)
	{
		switch (Command.Type)
		{
		case command_type::BIND_BUFFER:
			SendBindBuffer(Command.BindBuffer);
			break;

		case command_type::DRAW_ARRAY:
			SendDrawArray(Command.DrawArray);
			break;
		}
	}
	Commands.clear();
	
	// Reset state
	PrevDepthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
	PrevBlend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, PrevBlendSrcAlpha);

	glPopDebugGroup();
}

void wireframe_renderer::BindBuffer(GLuint MeshVBO, GLsizei PositionStride, GLsizei PositionOffset, int VertexCount)
{
	command Command;
	Command.Type = command_type::BIND_BUFFER;
	Command.BindBuffer = {};
	Command.BindBuffer.MeshVBO = MeshVBO;
	Command.BindBuffer.PositionStride = PositionStride;
	Command.BindBuffer.PositionOffset = PositionOffset;
	Command.BindBuffer.VertexCount = VertexCount;
	Commands.push_back(Command);
}

void wireframe_renderer::DrawArray(GLint First, GLsizei Count, const mat4& MVP)
{
	command Command;
	Command.Type = command_type::DRAW_ARRAY;
	Command.DrawArray = {};
	Command.DrawArray.First = First;
	Command.DrawArray.Count = Count;
	Command.DrawArray.MVP = MVP;
	Commands.push_back(Command);
}
