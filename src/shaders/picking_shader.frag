#version 330 core

// Fragment out
out vec4 oColor;

// Uniforms
uniform vec4 uPickingColor;

void main()
{
    oColor = uPickingColor;
}