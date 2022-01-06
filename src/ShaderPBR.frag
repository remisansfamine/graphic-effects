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

const float PI = 3.14159265359;

// Shader outputs
out vec4 oColor;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

//calculate the ratio between specular and diffuse reflection, 
//or how much the surface reflects light versus how much it refracts light
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

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
        float NDF = DistributionGGX(N, H, uMaterial.roughness);        
        float G   = GeometrySmith(N, V, L, uMaterial.roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - uMaterial.metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);    
        Lo += (kD * uMaterial.albedo / PI + uMaterial.specular) * radiance * NdotL; 
    }   

    vec3 ambient = vec3(0.03) * uMaterial.albedo * uMaterial.ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  //Gamma correction
    
    oColor = vec4(color, 1.0);
}