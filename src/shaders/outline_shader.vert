#version 330 core

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;

// Uniforms
uniform mat4 uModelViewProj;

out VS_OUT
{
    vec2 UVs;
}vs_out;

void main()
{
    vs_out.UVs = aUV;
    gl_Position = uModelViewProj * vec4(aPosition, 1.0);
}