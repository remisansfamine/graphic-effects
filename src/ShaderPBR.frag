//#version 330 core

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

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
    sampler2D normalMap;
    sampler2D albedoMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;

    bool isTextured;

    vec4 color;
    vec3  albedo;
    float metallic;
    float roughness;
    float ao;
};

#define LIGHT_COUNT 4
// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

uniform material uMaterial;



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

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(uMaterial.normalMap, vUV).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(vPos);
    vec3 Q2  = dFdy(vPos);
    vec2 st1 = dFdx(vUV);
    vec2 st2 = dFdy(vUV);

    vec3 N   = normalize(vNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPosition.xyz - vPos.xyz);

    vec3 albedo;
    vec3 normal;
    float metallic;
    float roughness;
    float ao;

    if (uMaterial.isTextured)
    {
        albedo    = pow(texture(uMaterial.albedoMap, vUV).rgb, vec3(2.2));
        N     = getNormalFromMap();
        metallic  = texture(uMaterial.metallicMap, vUV).r;
        roughness = texture(uMaterial.roughnessMap, vUV).r;
        ao        = texture(uMaterial.aoMap, vUV).r;
    }
    else
    {
        albedo    = uMaterial.albedo;
        metallic  = uMaterial.metallic;
        roughness = uMaterial.roughness;
        ao        = uMaterial.ao;
    }
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    // reflectance equation
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < LIGHT_COUNT; ++i) 
    {
        if (!uLight[i].enabled)
            continue;

        // calculate per-light radiance
        vec3 L = normalize(uLight[i].position.xyz - vPos.xyz);
        vec3 H = normalize(V + L);

        float distance = length(uLight[i].position.xyz - vPos.xyz);
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

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  //Gamma correction
    
    oColor = vec4(color, 1.0);
}