
#include <cassert>
#include <vector>
#include <string>
#include <map>

#include <stb_image.h>

#include "platform.h"
#include "mesh.h"

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

using namespace GL;

static const char* ShaderStructsDefinitionsStr = R"GLSL(
#line 19
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
    vec3(0.0, 0.0, 0.0),
    vec3(0.8, 0.8, 0.8),
    vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 0.0),
    32.0);
)GLSL";

// Light shader function
static const char* PhongLightingStr = R"GLSL(
#line 66
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
// PHONG SHADER STOP ===============
// =================================
)GLSL";

void GL::UniformLight(GLuint Program, const char* LightUniformName, const light& Light)
{
	glUseProgram(Program);
	char UniformMemberName[255];

	sprintf(UniformMemberName, "%s.enabled", LightUniformName);
	glUniform1i(glGetUniformLocation(Program, UniformMemberName), Light.Enabled);

	sprintf(UniformMemberName, "%s.viewPosition", LightUniformName);
	glUniform4fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Position.e);

	sprintf(UniformMemberName, "%s.ambient", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Ambient.e);

	sprintf(UniformMemberName, "%s.diffuse", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Diffuse.e);

	sprintf(UniformMemberName, "%s.specular", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Specular.e);

	sprintf(UniformMemberName, "%s.attenuation", LightUniformName);
	glUniform1fv(glGetUniformLocation(Program, UniformMemberName), 3, Light.Attenuation.e);
}

void GL::UniformMaterial(GLuint Program, const char* MaterialUniformName, const material& Material)
{
	glUseProgram(Program);
	char UniformMemberName[255];

	sprintf(UniformMemberName, "%s.ambient", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Ambient.e);

	sprintf(UniformMemberName, "%s.diffuse", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Diffuse.e);

	sprintf(UniformMemberName, "%s.specular", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Specular.e);

	sprintf(UniformMemberName, "%s.emission", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Emission.e);

	sprintf(UniformMemberName, "%s.shininess", MaterialUniformName);
	glUniform1f(glGetUniformLocation(Program, UniformMemberName), Material.Shininess);
}

GLuint GL::CompileShaderEx(GLenum ShaderType, int ShaderStrsCount, const char** ShaderStrs, bool InjectLightShading)
{
	GLuint Shader = glCreateShader(ShaderType);

	std::vector<const char*> Sources;
	Sources.reserve(4);
	Sources.push_back("#version 330 core\n");

	if (InjectLightShading)
	{
		Sources.push_back(ShaderStructsDefinitionsStr);
		Sources.push_back(PhongLightingStr);
	}
	for (int i = 0; i < ShaderStrsCount; ++i)
		Sources.push_back(ShaderStrs[i]);

	glShaderSource(Shader, (GLsizei)Sources.size(), &Sources[0], nullptr);
	glCompileShader(Shader);

	GLint CompileStatus;
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompileStatus);
	if (CompileStatus == GL_FALSE)
	{
		char Infolog[1024];
		glGetShaderInfoLog(Shader, ARRAY_SIZE(Infolog), nullptr, Infolog);
		fprintf(stderr, "Shader error: %s\n", Infolog);
	}

	return Shader;
}

GLuint GL::CompileShader(GLenum ShaderType, const char* ShaderStr, bool InjectLightShading)
{
	return GL::CompileShaderEx(ShaderType, 1, &ShaderStr, InjectLightShading);
}

GLuint GL::CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringsCount, const char** FSStrings, bool InjectLightShading)
{
	GLuint Program = glCreateProgram();

	GLuint VertexShader = GL::CompileShaderEx(GL_VERTEX_SHADER, VSStringsCount, VSStrings);
	GLuint FragmentShader = GL::CompileShaderEx(GL_FRAGMENT_SHADER, FSStringsCount, FSStrings, InjectLightShading);

	glAttachShader(Program, VertexShader);
	glAttachShader(Program, FragmentShader);

	glLinkProgram(Program);

	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);

	return Program;
}

GLuint GL::CreateProgram(const char* VSString, const char* FSString, bool InjectLightShading)
{
	return GL::CreateProgramEx(1, &VSString, 1, &FSString, InjectLightShading);
}

const char* GL::GetShaderStructsDefinitions()
{
	return ShaderStructsDefinitionsStr;
}

void GL::UploadTexture(const char* Filename, int ImageFlags, int* WidthOut, int* HeightOut)
{
    // Flip
    stbi_set_flip_vertically_on_load((ImageFlags & IMG_FLIP) ? 1 : 0);

    // Desired channels
    int DesiredChannels = 0;
	int Channels = 0;
	if (ImageFlags & IMG_FORCE_GREY)
	{
		DesiredChannels = STBI_grey;
		Channels = 1;
	}
	if (ImageFlags & IMG_FORCE_GREY_ALPHA)
	{
		DesiredChannels = STBI_grey_alpha;
		Channels = 2;
	}
	if (ImageFlags & IMG_FORCE_RGB)
	{
		DesiredChannels = STBI_rgb;
		Channels = 3;
	}
	if (ImageFlags & IMG_FORCE_RGBA)
	{
		DesiredChannels = STBI_rgb_alpha;
		Channels = 4;
	}

    // Loading
    int Width, Height;
    uint8_t* Image = stbi_load(Filename, &Width, &Height, (DesiredChannels == 0) ? &Channels : nullptr, DesiredChannels);
    if (Image == nullptr)
    {
        fprintf(stderr, "Image loading failed on '%s'\n", Filename);
        return;
    }

	GLint GLImageFormat[] = 
	{
		-1, // 0 Channels, unused
		GL_RED,
		GL_RG,
		GL_RGB,
		GL_RGBA
	};

    // Uploading
    glTexImage2D(GL_TEXTURE_2D, 0, GLImageFormat[Channels], Width, Height, 0, GLImageFormat[Channels], GL_UNSIGNED_BYTE, Image);
    stbi_image_free(Image);

    // Mipmaps
    if (ImageFlags & IMG_GEN_MIPMAPS)
        glGenerateMipmap(GL_TEXTURE_2D);

    if (WidthOut)
        *WidthOut = Width;

    if (HeightOut)
        *HeightOut = Height;

    stbi_set_flip_vertically_on_load(0); // Always reset to default value
}

void GL::UploadCheckerboardTexture(int Width, int Height, int SquareSize)
{
	std::vector<v4> Texels(Width * Height);

	for (int y = 0; y < Height; ++y)
	{
		for (int x = 0; x < Width; ++x)
		{
			int PixelIndex = x + y * Width;
			int TileX = x / SquareSize;
			int TileY = y / SquareSize;
			Texels[PixelIndex] = ((TileX + TileY) % 2) ? v4{ 0.1f, 0.1f, 0.1f, 1.f } : v4{ 0.3f, 0.3f, 0.3f, 1.f };
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Width, 0, GL_RGBA, GL_FLOAT, &Texels[0]);
}
