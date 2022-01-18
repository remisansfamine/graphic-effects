#include "structures.h"

namespace RBS
{
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
    }
}