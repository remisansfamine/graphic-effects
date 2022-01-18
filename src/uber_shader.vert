#version 330 core

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space
out mat3 vTBN;

void main()
{
    vec4 pos4 = uModel * vec4(aPosition, 1.0);
    vPos = vec3(pos4);
    vUV = aUV;

    vec3 T = normalize(mat3(uModelNormalMatrix) * aTangent);
    vec3 N = normalize(mat3(uModelNormalMatrix) * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    vTBN = transpose(mat3(T, B, N));

    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;

    gl_Position = uProjection * uView * pos4;
}