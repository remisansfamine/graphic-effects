out vec4 FragColor;
in  vec3 TexCoords;

uniform samplerCube skyTexture;

void main()
{    
    FragColor = texture(skyTexture, TexCoords);
}