layout (location = 0) in vec3 VertPos;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

void main()
{
	gl_Position = uLightSpaceMatrix * uModel * vec4(VertPos, 1.0);
}