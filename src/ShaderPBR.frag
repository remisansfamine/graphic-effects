//#version 330 core

// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in mat3 vTBN;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

mat3 TBN;
vec3 Pos;

// Light structure
struct light
{
	int lightType; //off/dir/point/spot
    vec4 position;
    vec3 direction;
    vec3 diffuse;
    vec3 params; //cutOff/outerCutOff/intensity

};

struct material
{
    sampler2D normalMap;
    sampler2D albedoMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;

    bool hasNormalMap;

    vec3  albedo;
    float specular;
    float metallic;
    float roughness;
    float ao;

    float clearCoat;
    float clearCoatRoughness;
};

#define LIGHT_COUNT 4
// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

uniform material uMaterial;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;  

uniform bool hasIrradianceMap;

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
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

float V_Kelemen(float clearCoatRoughness, float LdotH)
{
    return clearCoatRoughness / (LdotH * LdotH);
}


//Specular IBL
vec3 getIBLRadianceGGX(float NdotV, vec3 R, float roughness, vec3 F0, float specularWeight)
{
    vec2 brdfSamplePoint = clamp(vec2(NdotV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    vec2 envBRDF = texture(brdfLUT, brdfSamplePoint).rg;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;   

    vec3 specularLight = prefilteredColor.rgb;

    vec3 kS = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 FssEss = kS * envBRDF.x + envBRDF.y;

    return specularWeight * specularLight * FssEss;
}

//Diffuse IBL
vec3 getIBLRadianceLambertian(float NdotV, vec3 N, float roughness, vec3 albedo, vec3 F0, float specularWeight)
{
    vec2 brdfSamplePoint = clamp(vec2(NdotV, roughness), vec2(0.0, 0.0), vec2(1.0, 1.0));
    vec2 envBRDF = texture(brdfLUT, brdfSamplePoint).rg;

    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 kS = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 FssEss = specularWeight * kS * envBRDF.x + envBRDF.y;

    // Multiple scattering, from Fdez-Aguera
    float Ems = (1.0 - (envBRDF.x + envBRDF.y));
    vec3 F_avg = specularWeight * (F0 + (1.0 - F0) / 21.0);
    vec3 FmsEms = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);
    vec3 kD = albedo * (1.0 - FssEss + FmsEms);

    return (FmsEms + kD) * irradiance;
}

float getLightAttenuation(int lightType, vec3 lightDirection, vec3 spotDirection, float cutOff, float outerCutOff)
{
    float attenuation = 1.0;

    if (lightType == 2) //point light
    {
        float distance = length(lightDirection);
        attenuation = 1.0 / (distance * distance);
    }
    else if (lightType == 3) //spotLight
    {
        float distance = length(lightDirection);
        attenuation = 1.0 / (distance * distance);

        float theta = -dot(normalize(lightDirection), normalize(spotDirection));
        float epsilon = cutOff - outerCutOff;

        attenuation = attenuation *  clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);
    }

    return attenuation;
}

vec3 getBDRFResult(vec3 V, vec3 N, vec3 albedo, float roughness, float metallic, vec3 F0, float specularWeight)
{
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < LIGHT_COUNT; ++i) 
    {
        if (uLight[i].lightType == 0)
            continue;

        vec3 lightPos = TBN * uLight[i].position.xyz;
        
        vec3 lightDirection = lightPos;

        if (uLight[i].lightType != 1)
            lightDirection -= Pos;
            
        float lightIntensity = uLight[i].params.z;

        // calculate per-light radiance
        vec3 L = normalize(lightDirection);
        vec3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0);
        float HdotV = max(dot(H, V), 0.0);
        float NdotV = max(dot(N, V), 0.0);

        float attenuation = getLightAttenuation(uLight[i].lightType, lightDirection, 
        TBN * uLight[i].direction, uLight[i].params.x, uLight[i].params.y);

        vec3 radiance = (lightIntensity * uLight[i].diffuse) * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = FresnelSchlick(HdotV, F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - specularWeight * kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        vec3 specular     = numerator / denominator;  

        Lo += ((kD * albedo / PI + specular * specularWeight) * radiance * NdotL); 
    } 

    return Lo;
}


vec3 getClearCoatBDRF(vec3 V, vec3 N, float clearCoatRoughness, vec3 F0)
{
    vec3 clearCoat = vec3(0.0);
     for(int i = 0; i < LIGHT_COUNT; ++i) 
    {
        if (uLight[i].lightType == 0)
            continue;

        vec3 lightPos = TBN * uLight[i].position.xyz;
        float lightIntensity = uLight[i].params.z;

        // calculate per-light radiance
        vec3 L = normalize(lightPos - Pos.xyz);
        vec3 H = normalize(V + L);

        float distance = length(lightPos - Pos.xyz);
        float attenuation = 1.0 / (distance * distance);

        float Dc = DistributionGGX(N, H, clearCoatRoughness);        
        float Vc = V_Kelemen(clearCoatRoughness, max(dot(L,H), 0.0));      
        vec3 Fc  = FresnelSchlick(max(dot(H, V), 0.0), F0);  
        vec3 Frc = Dc * Vc * Fc;

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);

        clearCoat += (Frc * NdotL ) * attenuation;
    }   

    return clearCoat;
}


void main()
{
    TBN = uMaterial.hasNormalMap ? vTBN : mat3(1.0);

    Pos = TBN * vPos;
    vec3 N = normalize(vNormal);
    vec3 V = normalize(TBN * uViewPosition.xyz - Pos.xyz);
    float NdotV = max(dot(N, V), 0.0);

    vec3 albedo = uMaterial.albedo * pow(texture(uMaterial.albedoMap, vUV).rgb, vec3(2.2));
    float metallic = uMaterial.metallic * texture(uMaterial.metallicMap, vUV).r;
    float roughness = uMaterial.roughness * texture(uMaterial.roughnessMap, vUV).r;
    float ao        = uMaterial.ao * texture(uMaterial.aoMap, vUV).r;
    float specularWeight = uMaterial.specular;

    if (uMaterial.hasNormalMap)
            N = texture(uMaterial.normalMap, vUV).xyz * 2.0 - 1.0;

    vec3 R = reflect(-V, N); 
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    /* -- BDRF -- */
    vec3 Lo = getBDRFResult(V, N, albedo, roughness, metallic, F0, specularWeight);
    /* --------- */


    /* -- IBL Irradiance -- */
    vec3 ambient;
    if (hasIrradianceMap)
    {
        vec3 specular = getIBLRadianceGGX(NdotV, R, roughness, F0, specularWeight);
        vec3 diffuse = getIBLRadianceLambertian(NdotV, N, roughness, albedo, F0, specularWeight);
        ambient = (specular + diffuse) * ao;
    }
    else //Lit
    {
        ambient = vec3(0.03) * albedo * ao;
    }
    /* ------------------- */

    /* -- Clear Coat --*/
    vec3 clearCoat = vec3(0.0);
    vec3 clearCoatFresnel = vec3(0.0);
    if (uMaterial.clearCoat != 0.0)
    {
        float clearCoatRoughness = clamp(uMaterial.clearCoatRoughness, 0.089, 1.0);
        clearCoatRoughness = pow(clearCoatRoughness, 2.0);

        clearCoat += getClearCoatBDRF(V, N, clearCoatRoughness, F0); //BDRF

        if (hasIrradianceMap)
            clearCoat += getIBLRadianceGGX(NdotV, R, uMaterial.clearCoatRoughness, F0, 1.0); //IBL

        //clearCoat
        clearCoat = clearCoat * uMaterial.clearCoat;
        clearCoatFresnel = FresnelSchlick(max(dot(N,V), 0.0), F0);
    }
    /* -------------- */

    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  //Gamma correction

    color = color * (1.0 - uMaterial.clearCoat * clearCoatFresnel) + clearCoat;
    oColor = vec4(color, 1.0);
}