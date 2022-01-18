#version 330 core

#define LIGHT_COUNT 6

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

struct light_shade_result
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	float shininess;
};
// Default light
light gDefaultLight = light(
	true,
    vec4(1.0, 2.5, 0.0, 1.0),
    vec3(0.2, 0.2, 0.2),
    vec3(0.8, 0.8, 0.8),
    vec3(0.9, 0.9, 0.9),
    vec3(1.0, 0.0, 0.0));

// Default material
uniform material gDefaultMaterial = material(
    vec3(0, 0, 0),
    vec3(0.8, 0.8, 0.8),
    vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 0.0),
    32.0);
// =================================
// PHONG SHADER START ===============

// Phong shading function (model-space)
light_shade_result light_shade(light light, float shininess, vec3 eyePosition, vec3 position, vec3 normal)
{
	light_shade_result r = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	if (!light.enabled)
		return r;

    vec3 lightDir;
    float lightAttenuation = 1.0;
    if (light.position.w > 0.0)
    {
        // Point light
        vec3 lightPosFromVertexPos = (light.position.xyz / light.position.w) - position;
        lightDir = normalize(lightPosFromVertexPos);
        float dist = length(lightPosFromVertexPos);
        lightAttenuation = 1.0 / (light.attenuation[0] + light.attenuation[1]*dist + light.attenuation[2]*light.attenuation[2]*dist);
    }
    else
    {
        // Directional light
        lightDir = normalize(light.position.xyz);
    }

    if (lightAttenuation < 0.001)
        return r;

    vec3 eyeDir  = normalize(eyePosition - position);
	vec3 reflectDir = reflect(-lightDir, normal);
	float specAngle = max(dot(reflectDir, eyeDir), 0.0);

    r.ambient  = lightAttenuation * light.ambient;
    r.diffuse  = lightAttenuation * light.diffuse  * max(dot(normal, lightDir), 0.0);
    r.specular = lightAttenuation * light.specular * (pow(specAngle, shininess / 4.0));
	r.specular = clamp(r.specular, 0.0, 1.0);

	return r;
}

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in mat3 vTBN;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uNormalMap;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

light_shade_result get_lights_shading(in vec3 normal)
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light currLight = uLight[i];
        currLight.position = vec4(vTBN * currLight.position.xyz, currLight.position.w);

        light_shade_result currLightResult = light_shade(currLight, gDefaultMaterial.shininess, vTBN * uViewPosition, vTBN * vPos, normal);
        lightResult.ambient  += currLightResult.ambient;
        lightResult.diffuse  += currLightResult.diffuse;
        lightResult.specular += currLightResult.specular;
    }
    return lightResult;
}

void main()
{
    vec3 normal = texture(uNormalMap, vUV).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    // Compute phong shading
    light_shade_result lightResult = get_lights_shading(normal);
    
    vec3 albedo = texture(uDiffuseTexture, vUV).rgb;

    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * albedo;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient * albedo;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor), 1.0);
}