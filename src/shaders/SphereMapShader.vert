#version 330 core

layout (location = 0) in vec3 aPosition;

out vec3 vPos;

uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    vPos = aPosition;  
    gl_Position =  uProjection * uView * vec4(vPos, 1.0);
}