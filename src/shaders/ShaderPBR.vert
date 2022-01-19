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
   vec3 T = normalize(mat3(uModel) * aTangent);
   vec3 N = normalize(mat3(uModel) * aNormal);
   T = normalize(T - dot(T, N) * N);
   vec3 B = cross(N, T);
   
   vTBN = mat3(T, B, N);

   vUV = aUV;
   vec4 pos4 = (uModel * vec4(aPosition, 1.0));
   vPos = pos4.xyz;
   vNormal = N;

   gl_Position = uProjection * uView * pos4;
}