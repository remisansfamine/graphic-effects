#version 330 core

// Varyings
in VS_OUT
{
    //vec2 UVs;
    //vec3 Pos;
    vec3 Normal;
    //vec3 ViewDir;
}fs_in;

// Uniforms
uniform vec3 uColor;
uniform vec4 uLightPos;

uniform bool uUsePalette;

uniform sampler2D uToonPalette;

// Shader outputs
out vec4 oColor;

vec3 ToonShadingPalette(vec3 LightPos, vec3 Normal)
{
    float NL = dot(normalize(LightPos),Normal);

    float intensity = clamp(NL, 0.0, 1.0);
    
    vec2 UVs = vec2(intensity, 0.0);

    return texture(uToonPalette, UVs).rgb;
}

vec3 ToonShading(vec3 LightPos, vec3 Normal)
{
    float intensity = dot(normalize(LightPos),Normal);

    if (intensity > 0.95)      return vec3(1.0);
    else if (intensity > 0.75) return vec3(0.8);
    else if (intensity > 0.50) return vec3(0.6);
    else if (intensity > 0.25) return vec3(0.4);
    else                       return vec3(0.2);
}

void main()
{
    vec3 colorAttenuation;

    if (uUsePalette)
        colorAttenuation = ToonShadingPalette(uLightPos.xyz, fs_in.Normal);
    else
        colorAttenuation = ToonShading(uLightPos.xyz, fs_in.Normal);

    oColor = vec4(uColor * colorAttenuation, 1.0);
}