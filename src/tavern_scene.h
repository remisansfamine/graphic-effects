#pragma once

#include <vector>

#include "opengl_helpers.h"

// Tavern scene data (mapped on GPU)
class tavern_scene
{
public:
    tavern_scene(GL::cache& GLCache);
    ~tavern_scene();
    
    // Mesh
    GLuint MeshBuffer = 0;
    int MeshVertexCount = 0;
    vertex_descriptor MeshDesc;

    // Lights buffer
    GLuint LightsUniformBuffer = 0;
    int LightCount = 8;

    // Textures
    GLuint DiffuseTexture = 0;
    GLuint EmissiveTexture = 0;

    // ImGui debug function to edit lights
    void InspectLights();

private:
    // Lights data
    std::vector<GL::light> Lights;
};