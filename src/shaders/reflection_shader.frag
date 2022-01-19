#version 330 core

out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform int onReflect;
uniform float refractRatio;
uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{             
    vec3 I = normalize(Position - cameraPos);

    vec3 R;
    if (onReflect > 0)
        R = reflect(I, normalize(Normal));
    else
        R = refract(I, normalize(Normal), refractRatio);

    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}