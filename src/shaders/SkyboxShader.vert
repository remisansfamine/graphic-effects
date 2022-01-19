#version 330 core

layout (location = 0) in vec3 aPosition;

uniform mat4 uProjection;
uniform mat4 uView;

out vec3 vPos;

void main()
{
    vPos = aPosition;

    mat4 rotView = mat4(mat3(uView)); // remove translation from the view matrix
    vec4 clipPos = uProjection * rotView * vec4(vPos, 1.0);

    gl_Position = clipPos.xyww;
}