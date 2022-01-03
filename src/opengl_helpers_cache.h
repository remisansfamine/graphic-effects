#pragma once

#include <string>
#include <vector>
#include <map>

#include "opengl_headers.h"
#include "mesh.h"

namespace GL
{
	class cache
	{
	public:
        cache();
        ~cache();
        GLuint LoadObj(const char* Filename, float Scale, int* VertexCountOut);
        GLuint LoadTexture(const char* Filename, int ImageFlags = 0, int* WidthOut = nullptr, int* HeightOut = nullptr);

	private:
		struct mesh
		{
			GLuint VertexBuffer;
			int Size;
		};

		struct texture_identifier
		{
			std::string Filename;
			int ImageFlags;

			bool operator<(const texture_identifier& Other) const
			{
				return Filename < Other.Filename;
			}
		};

		struct texture
		{
			GLuint TextureID;
			int Width;
			int Height;
		};

		std::vector<vertex_full> TmpBuffer;
		std::map<std::string, mesh> VertexBufferMap;
		std::map<texture_identifier, texture> TextureMap;
	};
}
