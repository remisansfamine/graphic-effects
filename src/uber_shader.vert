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

uniform vec3 uViewPosition;

// Light structure
struct light
{
	bool enabled;
    vec4 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

#define LIGHT_COUNT 6

out VS_OUT
{
    // Varyings
    vec2 UV;
    vec3 fragPos;      // Vertex position in view-space
    vec3 TSFragPos;      // Vertex position in tangent-space
    vec3 TSViewPos;      // Vertex position in tangent-space
    vec3 normal;        // Vertex normal in view-space
    mat3 TBN;
    vec3 TSLightsPos[LIGHT_COUNT];
} vs_out;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

void main()
{
    vs_out.UV = aUV;

    vec4 pos4 = uModel * vec4(aPosition, 1.0);
    vs_out.fragPos = vec3(pos4);

    vec3 T = normalize(mat3(uModelNormalMatrix) * aTangent);
    vec3 N = normalize(mat3(uModelNormalMatrix) * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    vs_out.TBN = transpose(mat3(T, B, N));

    vs_out.TSFragPos = vs_out.TBN * vs_out.fragPos;

    vs_out.TSViewPos = vs_out.TBN * uViewPosition;

    vs_out.normal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;

    gl_Position = uProjection * uView * pos4;
    
    for (int i = 0; i < LIGHT_COUNT; i++)
        vs_out.TSLightsPos[i] = vs_out.TBN * uLight[i].position.xyz;
}