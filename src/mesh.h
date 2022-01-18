#pragma once

#include <vector>

#include "types.h"

// Descriptor for interleaved vertex formats
struct vertex_descriptor
{
	int Stride;
	int PositionOffset;
	bool HasNormal;
	int NormalOffset;
	bool HasUV;
	int UVOffset;

	int TangentOffset;
	int BitangentOffset;
};

struct vertex_full_simple
{
	v3 Position;
	v3 Normal;
	v2 UV;
};

struct vertex_full
{
	v3 Position;
	v3 Normal;
	v2 UV;

	v3 Tangent;
	v3 Bitangent;
};

namespace Mesh
{
void  AddNormalMapParameters(std::vector<vertex_full>& Mesh);
void  AddNormalMapParameters(vertex_full* Mesh, int VertexCount);
void* Transform(void* Vertices, void* End, const vertex_descriptor& Descriptor, const mat4& Transform);
void* BuildQuad(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildScreenQuad(void* Vertices, void* End, const vertex_descriptor& Descriptor, const v2& screenSize);
void* BuildCube(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildInvertedCube(void* Vertices, void* End, const vertex_descriptor& Descriptor);
void* BuildSphere(void* Vertices, void* End, const vertex_descriptor& Descriptor, int Lon, int Lat);
void* LoadObj(void* Vertices, void* End, const vertex_descriptor& Descriptor, const char* Filename, float Scale);
bool LoadObjNoConvertion(std::vector<vertex_full>& Mesh, const char* Filename, float Scale);
}