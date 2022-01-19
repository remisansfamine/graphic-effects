#version 330 core

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out VS_OUT
{
    //vec2 UVs;
    //vec3 Pos;    
    vec3 Normal; 
    //vec3 ViewDir;
}vs_out;

void main()
{
    //vs_out.ViewDir = vec3(uView[0][3], uView[1][3], uView[2][3]);
    //vs_out.UVs = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    //vs_out.Pos = pos4.xyz;
    vs_out.Normal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
}