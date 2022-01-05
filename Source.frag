#version 330 core

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

// Light structure
struct light
{
	bool enabled;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

struct material
{
    vec3 ambient;
    vec3 albedo;
    vec3 specular;
    vec3 emission;

    vec3 normal;
    vec3 transmitance;

    float metallic;
    float shininess;
    float roughness;

    float ao; //Ambient occlusion
    float ior;  // index of refraction
    float alpha;  // 1 == opaque; 0 == fully transparent
};

#define LIGHT_COUNT 5
// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Uniform blocks
layout(std140) uniform uMaterialBlock
{
	material uMaterial;
};

// Shader outputs
out vec4 oColor;

float DistributionGGX(vec3 N, vec3 H, float roughness) {return 0;}
float GeometrySchlickGGX(float NdotV, float roughness) {return 0;}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {return 0;}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {return vec}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPosition - vPos);
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, uMaterial.albedo, uMaterial.metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < LIGHT_COUNT; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(uLight[i].position - vPos);
        vec3 H = normalize(V + L);

        float distance = length(uLight[i].position - vPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = uLight[i].diffuse * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   

    
    
    
    // Apply light color
    //oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
    //oColor =
}